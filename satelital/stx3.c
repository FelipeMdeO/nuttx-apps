/****************************************************************************
 * apps/satelital/stx3.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "stx3.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MIN_BURST_INTERVAL          0x3C        // 60 x 5 = 300 sec
#define MAX_BURST_INTERVAL          0x78        // 120 x 5 = 600 sec
#define NUMBER_BURST_ATTEMPTS       0x03
#define RF_CHANNEL                  0x02        // 0x00 = A, 0x01 = B, 0x02 = C, 0x03 = D

#define NUM_UART_SEND_ATTEMPS       3U
#define INIT_POSITION_PAYLOAD       3U

#define COMMAND_SIZE_GET_CONFIG     5U
#define RESPONSE_SIZE_GET_CONFIG    14U

#define COMMAND_SIZE_SETUP          14U
#define RESPONSE_SIZE_SETUP         5U

#define COMMAND_SIZE_BURST_STATE    5U
#define RESPONSE_SIZE_BURST_STATE   6U

#define COMMAND_SIZE_NEW_BURST      14U
#define RESPONSE_SIZE_NEW_BURST     5U

#define COMMAND_SIZE_ABORT_BURST    14U
#define RESPONSE_SIZE_ABORT_BURST   5U

#define COMMAND_SIZE_GET_ESN        5U
#define RESPONSE_SIZE_GET_ESN       9U

#define PAYLOAD_LEN                 9U
#define CRC_LEN                     2U

#define DELAY_IN_BURST_PROCESS      1425U

/****************************************************************************
 * Private Data
 ****************************************************************************/

static STX3_BURST_STATES stx3_state = BURST_AVAILABLE;
static bool stx3_init_status = false;

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

uint16_t inline crc16_lsb_calc(const uint8_t *src, uint8_t size);
static inline bool stx3_crc_is_valid(const uint8_t *response_ptr, uint8_t response_length);
static bool stx3_is_configured(void);
static bool stx3_config(void);
static bool send_command_uart(const uint8_t command_len, const uint8_t *command_ptr,
                              const uint8_t response_len, uint8_t *response_ptr);
static bool send_message(const uint8_t command_len, const uint8_t *command_ptr,
                         const uint8_t response_len, uint8_t *response_ptr);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

uint16_t inline crc16_lsb_calc(const uint8_t *src, uint8_t size)
{
    uint16_t data;
    uint16_t crc;
    uint8_t i;

    if (size == 0)
    {
        return 0;
    }

    data = 0U;
    crc = UINT16_MAX;

    do
    {
        data = ((uint16_t) 0x00FF & *src++);
        crc = crc ^ data;

        for (i = 8U; i > 0; i--)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0x8408;
            else
                crc >>= 1;
        }
    } 
    while (--size);

    return ~crc;
}

static inline bool stx3_crc_is_valid(const uint8_t *response_ptr, uint8_t response_length)
{
    uint16_t calc_crc;
    uint16_t resp_crc; 

    calc_crc = crc16_lsb_calc(response_ptr, ((uint8_t)(response_length - CRC_LEN)));
    resp_crc = (uint16_t)(((uint16_t)response_ptr[(response_length - 1U)] << 8) | 
                            (uint16_t)response_ptr[response_length - CRC_LEN]);

    return (calc_crc == resp_crc);
}

static bool stx3_is_configured(void)
{
    const uint8_t query_config_cmd[COMMAND_SIZE_GET_CONFIG] = { 0xAA, 0x05, 0x07, 0x66, 0xB0 };
    uint8_t response[RESPONSE_SIZE_GET_CONFIG]              = {0};
    bool comm_ok = false;
    uint8_t i;

    // Try sending the query_config_cmd via UART
    for (i = 0; i < NUM_UART_SEND_ATTEMPS; i++)
    {
        // Wait longer for the next attempt if it is in the burst process
        if (stx3_state != BURST_AVAILABLE)
        {
            usleep(DELAY_IN_BURST_PROCESS * 1000);
        }

        if (send_command_uart(COMMAND_SIZE_GET_CONFIG, query_config_cmd, RESPONSE_SIZE_GET_CONFIG, response) == true)
        {
            comm_ok = true;
            break;
        }
    }

    if ( (comm_ok == true) && (crc_is_valid(response, RESPONSE_SIZE_GET_CONFIG) == true) )
    {
        if ((response[0x00] == 0xAA) && (response[0x01] == 0x0E) && (response[0x02] == 0x07))
        {
            if ( (response[0x07] != RF_CHANNEL)             ||
                (response[0x08] != NUMBER_BURST_ATTEMPTS)   ||
                (response[0x09] != MIN_BURST_INTERVAL)      ||
                (response[0x0A] != MAX_BURST_INTERVAL) )
            {
                printf("module_stx3.c | stx3_is_configured -> Settings divergent configuration\n");
                comm_ok = false;
            }
            else
            {
                comm_ok = true;
            }
        }
        else
        {
            printf("stx3_is_configured -> Invalid response format\n");
            comm_ok = false;
        }
    }
    else
    {
        printf("stx3_is_configured -> Invalid CRC response\n");
        comm_ok = false;
    }

    return comm_ok;
}

static bool stx3_config(void) 
{
    uint8_t setup_cmd[COMMAND_SIZE_SETUP]   = { 0xAA, 0x0E, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x18, 0x30, 0x00, 0xCE, 0x9C };
    uint8_t response[RESPONSE_SIZE_SETUP]   = {0};
    bool comm_ok = false;
    uint16_t crc;
    uint8_t i;

    setup_cmd[7U]     = RF_CHANNEL;
    setup_cmd[8U]     = NUMBER_BURST_ATTEMPTS;
    setup_cmd[9U]     = MIN_BURST_INTERVAL;
    setup_cmd[10U]    = MAX_BURST_INTERVAL;

    
    crc = crc16_lsb_calc(setup_cmd, (COMMAND_SIZE_SETUP - 2U));

    setup_cmd[12U] = (uint8_t)(crc & 0xFF);        // CRC low byte
    setup_cmd[13U] = (uint8_t)((crc >> 8) & 0xFF); // CRC high

    for (i = 0; i < NUM_UART_SEND_ATTEMPS; i++)
    {
        if (stx3_state != BURST_AVAILABLE)
        {
            usleep( DELAY_IN_BURST_PROCESS * 1000 );
        }

        if (send_command_uart(COMMAND_SIZE_SETUP, setup_cmd, RESPONSE_SIZE_SETUP, response) == true)
        {
            comm_ok = true;
            break;
        }
    }

    if (comm_ok == true)
    {
        if (crc_is_valid(response, RESPONSE_SIZE_SETUP))
        {
            if ((response[0U] == 0xAA) && (response[1U] == 0x05) && (response[2U] == 0x06))
            {
                comm_ok = true;
            }
            else
            {
                printf("stx3_config -> Invalid response format\n");
                comm_ok = false;
            }
        }
        else
        {
            debug_print("stx3_config -> Invalid CRC response\n");
            comm_ok = false;
        }
    }

    return comm_ok;    
}

static bool send_command_uart(const uint8_t command_len, const uint8_t *command_ptr,
                              const uint8_t response_len, uint8_t *response_ptr)
{

    uint32_t j;
    struct uart_connector *uart = get_usart0_connector();

    if (uart == NULL)
    {
        printf("send_command_uart -> UART connector is NULL\n");
        return false;
    }

    if (uart->write(command_ptr, command_len) == false)
    {
        printf("send_command_uart -> Failed to send the command\n");
        return false;
    }

    for (j = 0; j < response_len; j++)
    {
        if (uart->read(&response_ptr[j], 1U) == false)
        {
            debug_print("send_command_uart -> Failed to read the response\r\n");
            return false;
        }
    }

    return true;
}

static bool send_message(const uint8_t command_len, const uint8_t *command_ptr,
                         const uint8_t response_len, uint8_t *response_ptr)
{
    bool comm_ok = false;
    uint8_t i;

    for (i = 0; i < NUM_UART_SEND_ATTEMPS; i++)
    {
        if (stx3_state != BURST_AVAILABLE)
        {
            usleep(DELAY_IN_BURST_PROCESS * 1000);
        }

        if (send_command_uart(command_len, command_ptr, response_len, response_ptr) == true)
        {
            comm_ok = true;
            break;
        }
    }

    if (comm_ok == false)
    {
        stx3_reset();
        
        comm_ok = (send_command_uart(COMMAND_SIZE_GET_CONFIG, command_ptr, RESPONSE_SIZE_GET_CONFIG, response_ptr) == true);
    }

    return comm_ok;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

STX3_BURST_STATES stx3_new_burst(
        const uint8_t *payload_ptr,
        uint8_t payload_len,
        uint32_t *buffer_error_msg,
        uint32_t buffer_error_size
        )
{
    /* Command template with placeholder for payload and CRC */
    uint8_t send_data_cmd[COMMAND_SIZE_NEW_BURST]   = { 0xAA, 0x0E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xBE, 0xE8 };
    uint8_t response[RESPONSE_SIZE_NEW_BURST]       = {0};

    STX3_BURST_STATES response_state = BURST_AVAILABLE;
    uint8_t initial_position_payload = INIT_POSITION_PAYLOAD;

    /* Check the payload pointer */
    if ((payload_ptr == NULL) || (payload_len != PAYLOAD_LEN))
    {
        const char error[] = "module_stx3.c | stx3_new_burst -> payload_ptr OR payload_len ERROR\r\n";
        size_t error_len = strlen(error);
        debug_print("%s", error);

        if (error_len < buffer_error_size)
        {
            snprintf((char *)buffer_error_msg, error_len, "%s", error);
        }
        response_state = BURST_ERROR;
    }

    if (response_state == BURST_AVAILABLE) {

        for (uint8_t i = 0U; i < payload_len; i++)
        {
            send_data_cmd[initial_position_payload] = payload_ptr[i];
            initial_position_payload++;
        }

        /* Calculate CRC */
        uint16_t crc = crc16_lsb_calc(send_data_cmd, (COMMAND_SIZE_SETUP - 2U));

        send_data_cmd[12U] = (uint8_t)(crc & 0xFF);        // CRC low byte
        send_data_cmd[13U] = (uint8_t)((crc >> 8) & 0xFF); // CRC high

        /* Try sending the send_data_cmd via UART */
        bool res_send_message = send_message(COMMAND_SIZE_NEW_BURST, send_data_cmd, RESPONSE_SIZE_NEW_BURST, response);

        /* Calculation CRC and check */
        bool res_crc_is_valid = stx3_crc_is_valid(response, RESPONSE_SIZE_NEW_BURST);

        /* Check if communication was successful and CRC is valid */
        if ( (res_send_message == true) && (res_crc_is_valid == true) )
        {
            if ((response[0U] == 0xAA) && (response[1U] == 0x05) && (response[2U] == 0x00))
            {
                response_state = BURST_RUNNING;
                stx3_state = BURST_RUNNING;
            }
            else
            {
                const char error[] = "module_stx3.c | stx3_new_burst -> Invalid payload or response format\r\n";
                debug_print("%s", error);
                size_t error_len = strlen(error);

                if (error_len < buffer_error_size)
                {
                    snprintf((char *)buffer_error_msg, error_len, "%s", error);
                }

                response_state = BURST_ERROR;
            }
        }
        else
        {
            const char error[] = "module_stx3.c | stx3_new_burst -> Invalid CRC or command error\r\n";
            debug_print("%s", error);
            uint32_t error_len = strlen(error);

            if (error_len < buffer_error_size)
            {
                snprintf((char *)buffer_error_msg, error_len, "%s", error);
            }
            response_state = BURST_ERROR;
        }
    }

    return response_state;
}

STX3_BURST_STATES stx3_get_burst_state(void)
{
    /* Command to request the burst state */
    const uint8_t query_bursts_remaining_cmd[COMMAND_SIZE_BURST_STATE]  = {0xAA, 0x05, 0x04, 0xFD, 0x82};
    uint8_t response[RESPONSE_SIZE_BURST_STATE]                         = {0};

    STX3_BURST_STATES response_state = BURST_ERROR;

    /* Try sending the query_bursts_remaining_cmd via UART */
    bool res_send_message = send_message(COMMAND_SIZE_BURST_STATE, query_bursts_remaining_cmd, RESPONSE_SIZE_BURST_STATE, response);

    /* Calculation CRC and check */
    bool res_crc_is_valid = stx3_crc_is_valid(response, RESPONSE_SIZE_BURST_STATE);

    /* Check if communication was successful and CRC is valid */
    if ( (res_send_message == true) && (res_crc_is_valid == true) )
    {
        if ( (response[0] == 0xAA) && (response[1] == 0x06) && (response[2] == 0x04) )
        {
            uint8_t bursts_remaining = response[3];

            if (bursts_remaining > 0)
            {
                response_state = BURST_RUNNING;
                stx3_state = BURST_RUNNING;
            }
            else
            {
                response_state = BURST_AVAILABLE;
                stx3_state = BURST_AVAILABLE;
            }
        }
        else
        {
            const char error[] = "module_stx3.c | stx3_get_burst_state -> Invalid response format\r\n";
            debug_print("%s", error);
            response_state = BURST_ERROR;
        }
    }
    else
    {
        const char error[] = "module_stx3.c | stx3_get_burst_state -> Invalid CRC or command error\r\n";
        debug_print("%s", error);
        response_state = BURST_ERROR;
    }

    return response_state;
}

STX3_BURST_STATES stx3_abort_burst(void)
{
    /* Command to abort the burst transmission */
    const uint8_t abort_burst_cmd[COMMAND_SIZE_ABORT_BURST] = { 0xAA, 0x05, 0x03, 0x42, 0xF6 };
    uint8_t response[RESPONSE_SIZE_ABORT_BURST]             = {0};

    STX3_BURST_STATES response_state = BURST_ERROR;

    // Try sending the abort_burst_cmd via UART
    bool res_send_message = send_message(COMMAND_SIZE_ABORT_BURST, abort_burst_cmd, RESPONSE_SIZE_ABORT_BURST, response);

    /* Calculation CRC and check */
    bool res_crc_is_valid = stx3_crc_is_valid(response, RESPONSE_SIZE_ABORT_BURST);

    /* Check if communication was successful and CRC is valid */
    if ( (res_send_message == true) && (res_crc_is_valid == true) )
    {
        if ( (response[0] == 0xAA) && (response[1] == 0x05) && (response[2] == 0x03) )
        {
            response_state = BURST_AVAILABLE;
            stx3_state = BURST_AVAILABLE;
        }
        else
        {
            const char error[] = "module_stx3.c | stx3_abort_burst -> Invalid response format\r\n";
            debug_print("%s", error);

            response_state = BURST_ERROR;
        }
    }
    else
    {
        const char error[] = "module_stx3.c | stx3_abort_burst -> Invalid CRC or command error\r\n";
        debug_print("%s", error);
        response_state = BURST_ERROR;
    }

    return response_state;
}

uint32_t stx3_get_esn(void) 
{
    /* Command to request the ESN */
    const uint8_t query_esn_cmd[COMMAND_SIZE_GET_ESN]   = { 0xAA, 0x05, 0x01, 0x50, 0xD5 };
    uint8_t response[RESPONSE_SIZE_GET_ESN]             = {0};
    uint32_t esn_stx3 = UINT32_MAX;

    /* Try sending the query_esn_cmd via UART */
    bool res_send_message = send_message(COMMAND_SIZE_GET_ESN, query_esn_cmd, RESPONSE_SIZE_GET_ESN, response);

    /* Calculation CRC and check */
    bool res_crc_is_valid = stx3_crc_is_valid(response, RESPONSE_SIZE_GET_ESN);

    /* Check if communication was successful and CRC is valid */
    if ( (res_send_message == true) && ( res_crc_is_valid == true) ) 
    {
        if (response[0U] == 0xAA && response[1U] == 0x09 && response[2U] == 0x01)
        {
            /* Extract the ESN from the response */
            esn_stx3 = ((uint32_t)(response[3U] << 24) | (response[4U] << 16) | (response[5U] << 8) | response[6U]);
        }
        else
        {
            debug_print("module_stx3.c | stx3_get_esn -> Invalid response format\r\n");
            esn_stx3 = UINT32_MAX;
        }
    }

    return esn_stx3;
}

bool stx3_init(void)
{
    bool setup_flag = false;
#ifdef USE_GG11_DEV_KIT
    struct uart_connector *uart = get_usart0_connector();
#else
    struct uart_connector *uart = get_uart1_connector();
#endif
    uart->init(true);

    /* Turns on power to the STX3 module. */
    stx3_enable();
    usleep(1000 * 1000); /* Note - this delay was request from Global Star application engineer  */

    // Retrieves module state
    stx3_state = stx3_get_burst_state();

    /* Try configuring the STX3 module several times if you have an error */
    for (uint8_t i = 0; i < NUM_UART_SEND_ATTEMPS; i++)
    {
        if (stx3_config() == true)
        {
            setup_flag = true;
            break;
        }
    }
    
    /* If configuration was successful, it requests parameters for comparison */
    if (setup_flag == true) 
    {
        if (stx3_is_configured() == false)
        {
            debug_print("module_stx3.c | stx3_init -> Error on stx3_init(void)\r\n");
            setup_flag = false;
        }
        else
        {
            setup_flag = true;
        }
    }
    else 
    {
        debug_print("module_stx3.c | stx3_init -> Error on stx3_init(void)\r\n");
        setup_flag = false;
    }

    stx3_init_status = true;
    return setup_flag;
}


bool stx3_get_init_status(void)
{
    return stx3_init_status;
}

STX3_BURST_STATES stx3_get_state(void)
{
    return stx3_state;
}

void stx3_reset(void)
{
    debug_print("module_stx3.c | send_message -> Reset STX3 module\r\n");

    /* Reset the STX3 module. */
    stx3_disable();
    drv_timer_ms_delay(100);

    stx3_enable();
    drv_timer_ms_delay(1000);
}

