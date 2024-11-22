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
#include <nuttx/config.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "stx3.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define INIT_POSITION_PAYLOAD       3U

#define COMMAND_SIZE_GET_CONFIG     5U
#define RESPONSE_SIZE_GET_CONFIG    14U

#define COMMAND_SIZE_SETUP          14U
#define RESPONSE_SIZE_SETUP         5U

#define COMMAND_SIZE_BURST_STATE    5U
#define RESPONSE_SIZE_BURST_STATE   6U

#define RESPONSE_SIZE_NEW_BURST     5U

#define COMMAND_SIZE_ABORT_BURST    5U
#define RESPONSE_SIZE_ABORT_BURST   5U

#define COMMAND_SIZE_GET_ESN        5U
#define RESPONSE_SIZE_GET_ESN       9U

#define PAYLOAD_LEN                 9U
#define CRC_LEN                     2U

#define STX3_DEVNAME "/dev/ttyS1" //CONFIG_STX3_UART_DEVNAME

typedef enum {
    BURST_RUNNING,
    BURST_AVAILABLE,
    BURST_ERROR
} STX3_BURST_STATES;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static inline uint16_t crc16_lsb_calc(const uint8_t *src, uint8_t size);
static inline bool stx3_crc_is_valid(const uint8_t *response_ptr, uint8_t response_length);
static bool stx3_exchange(const uint8_t command_len, const uint8_t *command_ptr,
                           const uint8_t response_len, uint8_t *response_ptr);
static STX3_BURST_STATES stx3_get_burst_state(void);
static bool send_uart_data(const uint8_t *data, size_t len);
static bool read_uart_data(uint8_t *data, size_t len);
static inline void stx3_disable(void);
static inline void stx3_enable(void);
static void print_buffer_hex(const uint8_t *buffer, size_t size);
/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void stx3_disable(void)
{
    printf("stx3_disable called\n");
}

static inline void stx3_enable(void)
{
    printf("stx3_enable called\n");
}

static inline uint16_t crc16_lsb_calc(const uint8_t *src, uint8_t size)
{
  uint16_t data;
  uint16_t crc;
  uint8_t i;

  if (size == 0)
  {
    return 0;
  }

  data = 0U;
  crc = 0xFFFF;

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

  calc_crc = crc16_lsb_calc(response_ptr, (response_length - CRC_LEN) );
  resp_crc = (uint16_t)(((uint16_t)response_ptr[(response_length - 1U)] << 8) | 
                        (uint16_t)response_ptr[response_length - CRC_LEN]);

  if (calc_crc == resp_crc) 
  { 
    printf("CRC is valid\n");
    return OK;
  } 
  
  printf("calc_crc: %04X, resp_crc: %04X\n", calc_crc, resp_crc);
  printf("CRC is invalid\n");
  return ERROR;
}

static bool send_uart_data(const uint8_t *data, size_t len)
{
  ssize_t bytes_written;
  int fd;

  fd = open(STX3_DEVNAME, O_WRONLY);
  if (fd < 0)
  {
      printf("Failed to open UART device");
      return ERROR;
  }

  bytes_written = write(fd, data, len);

  if (bytes_written != (ssize_t)len)
  {
    fprintf(stderr, "Failed to write data to UART: %d\n", errno);
    close(fd);
    return ERROR;
  }

  close(fd);
  return OK;
}

static bool read_uart_data(uint8_t *data, size_t len)
{
  ssize_t bytes_read;
  int fd;

  fd = open(STX3_DEVNAME, O_RDONLY);
  if (fd < 0)
  {
      printf("Failed to open UART device");
      return ERROR;
  }

  bytes_read = read(fd, data, len);
  
  if (bytes_read != (ssize_t)len)
  {
    fprintf(stderr, "Failed to read data from UART: %d\n", errno);
    close(fd);
    return ERROR;
  }

  close(fd);
  return OK;
}

static bool stx3_exchange(const uint8_t command_len, const uint8_t *command_ptr,
                          const uint8_t response_len, uint8_t *response_ptr)
{

  if (send_uart_data(command_ptr, command_len) != OK)
  {
    return false;
  }

  if (read_uart_data(response_ptr, response_len) != OK)
  {
    return false;
  }

  printf("Sending : ");
  print_buffer_hex(command_ptr, command_len);
  printf("Received: ");
  print_buffer_hex(response_ptr, response_len);

  return stx3_crc_is_valid(response_ptr, response_len);
}

static STX3_BURST_STATES stx3_get_burst_state(void)
{
  /* Command to request the burst state */

  const uint8_t query_bursts_remaining_cmd[COMMAND_SIZE_BURST_STATE]  = {0xAA, 0x05, 0x04, 0xFD, 0x82};
  uint8_t response[RESPONSE_SIZE_BURST_STATE]                         = {0};
  uint8_t bursts_remaining;

  /* Check if communication was successful and CRC is valid */

  if ( stx3_exchange(COMMAND_SIZE_BURST_STATE, query_bursts_remaining_cmd, RESPONSE_SIZE_BURST_STATE, response) == OK)
  {
    bursts_remaining = response[3];

    if (bursts_remaining > 0)
    {
      printf("Bursts remaining: %d\n", bursts_remaining);
      return BURST_RUNNING;
    }

    else
    {
      printf("Bursts available\n");
      return BURST_AVAILABLE;
    }        
  }

  printf("Error getting burst state\n");
  return BURST_ERROR;
}

static void print_buffer_hex(const uint8_t *buffer, size_t size)
{
  for (size_t i = 0; i < size; i++)
  {
    printf("%02X ", buffer[i]);
  }

  printf("\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stx3_new_burst(const uint8_t *payload_ptr, uint8_t payload_len)
{

  printf("Inside stx3_new_burst\n");
  printf("Payload length: %d\n", payload_len);
  print_buffer_hex(payload_ptr, payload_len);

  // Entendo que command_size vai ser o payload len + o cabeçalho e o CRC
  // Mudar a implementação para malloc.

  uint8_t *send_data_cmd;
  uint8_t response[RESPONSE_SIZE_NEW_BURST]       = {0};
  uint8_t message_size;
  uint16_t crc;

  if ( stx3_get_burst_state() != BURST_AVAILABLE)
  {
    printf("Cannot send data - STX is not available, \n");
    return ERROR;
  }

  /* Check the payload pointer */

  if ((payload_ptr == NULL))
  {
    return ERROR;
  }

  /* Add preamble size and crc size to payload len */

  message_size = payload_len + 1 + 2;

  send_data_cmd = calloc(message_size, sizeof(uint8_t));

  if (send_data_cmd == NULL)
  {
    return ERROR;
  }

  send_data_cmd[0] = 0xAA;
  send_data_cmd[1] = message_size;
  send_data_cmd[2] = 0x00;

  memcpy(&send_data_cmd[INIT_POSITION_PAYLOAD], payload_ptr, payload_len);

  printf("Payload: ");
  print_buffer_hex(send_data_cmd, message_size);

  crc = crc16_lsb_calc(send_data_cmd, (message_size - 2U));

  send_data_cmd[message_size-2] = (uint8_t)(crc & 0xFF);        // CRC low byte
  send_data_cmd[message_size-1] = (uint8_t)((crc >> 8) & 0xFF); // CRC high

  print_buffer_hex(send_data_cmd, message_size);

  if ( stx3_exchange(message_size, send_data_cmd, RESPONSE_SIZE_NEW_BURST, response) == OK )
  {
    free(send_data_cmd);
    return OK;
  }

  free(send_data_cmd);
  return ERROR;

}

int stx3_abort_burst(void)
{
  /* Command to abort the burst transmission */

  const uint8_t abort_burst_cmd[RESPONSE_SIZE_ABORT_BURST] = { 0xAA, 0x05, 0x03, 0x42, 0xF6 };
  uint8_t response[RESPONSE_SIZE_ABORT_BURST]             = {0};

  /* Check if communication was successful and CRC is valid */

  if ( stx3_exchange(RESPONSE_SIZE_ABORT_BURST, abort_burst_cmd, RESPONSE_SIZE_ABORT_BURST, response) == OK)
  {
      return OK;
  }

  return ERROR;
}

uint32_t stx3_get_esn(void) 
{
  /* Command to request the ESN */

  const uint8_t query_esn_cmd[COMMAND_SIZE_GET_ESN]   = { 0xAA, 0x05, 0x01, 0x50, 0xD5 };
  uint8_t response[RESPONSE_SIZE_GET_ESN]             = {0};
  
  /* Check if communication was successful and CRC is valid */

  if ( stx3_exchange(COMMAND_SIZE_GET_ESN, query_esn_cmd, RESPONSE_SIZE_GET_ESN, response) == OK ) 
  {
      /* Extract the ESN from the response */

      return ((uint32_t)(response[3U] << 24) | (response[4U] << 16) | (response[5U] << 8) | response[6U]);

  }

  return __UINT32_MAX__;
}

void stx3_reset(void)
{
  printf("Reseting STX3 module\n");

  stx3_disable();
 
  usleep(100 * 1000);

  stx3_enable();

  usleep(100 * 1000);
}


int stx3_configure(uint8_t channel, uint8_t num_bursts, uint8_t min_interval, uint8_t max_interval) 
{
  uint8_t setup_cmd[COMMAND_SIZE_SETUP]   = { 0xAA, 0x0E, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x18, 0x30, 0x00, 0xCE, 0x9C };
  uint8_t response[RESPONSE_SIZE_SETUP]   = {0};
  uint16_t crc;

  setup_cmd[7U]     = channel;
  setup_cmd[8U]     = num_bursts;
  setup_cmd[9U]     = min_interval;
  setup_cmd[10U]    = max_interval;

  /* Change STX3 module to a know state. */

  stx3_reset();

  /* Note - The delay below was request from Global Star Application Team  */

  usleep(1000 * 1000);

  crc = crc16_lsb_calc(setup_cmd, (COMMAND_SIZE_SETUP - 2U));

  setup_cmd[12U] = (uint8_t)(crc & 0xFF);        // CRC low byte
  setup_cmd[13U] = (uint8_t)((crc >> 8) & 0xFF); // CRC high

  if (stx3_exchange(COMMAND_SIZE_SETUP, setup_cmd, RESPONSE_SIZE_SETUP, response) == OK)
  {
    return OK;
  }

  return -EXIT_FAILURE; 
}
