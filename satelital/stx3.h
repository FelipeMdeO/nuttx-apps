
/****************************************************************************
 * apps/satelital/stx3.h
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
#ifndef __APPS_SATELITAL_STX3_H
#define __APPS_SATELITAL_STX3_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef int (*cmd_t)(int argc, char **argv);

struct cmdmap_s
{
  const char *cmd;        /* Name of the command */
  cmd_t           handler;    /* Function that handles the command */
  const char *desc;       /* Short description */
  const char *usage;      /* Usage instructions for 'help' command */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initiates a new burst of transmission with the given payload.
 * 
 * This function creates a command to initiate a new transmission, 
 * calculates the CRC for the command, and sends it via UART. It verifies 
 * the response to determine if the burst transmission was successfully started.
 *
 * @param payload_ptr Pointer to the payload data to be included in the burst command.
 * @param payload_len Length of the payload data in bytes.
 * @return The state of the burst transmission after attempting to send the command.
 *         Possible return values are:
 *         - OK
 *         - ERROR
 */

int stx3_new_burst(const uint8_t *payload_ptr, uint8_t payload_len);

/**
 * @brief Abort the current burst transmission.
 * 
 * This function sends a command to abort the current burst transmission via UART,
 * and interprets the response to determine if the burst transmission was successfully aborted.
 *
 * @return The state of the burst transmission after attempting to abort it.
 *         Possible return values are:
 *         - BURST_AVAILABLE: The burst transmission was successfully aborted.
 *         - BURST_ERROR: An error occurred while attempting to abort the burst transmission.
 */

int stx3_abort_burst(void);

/**
 * @brief Retrieve the ESN (Electronic Serial Number) from the STX3 module.
 * 
 * This function sends a command to request the ESN from the STX3 module via UART,
 * and interprets the response to extract the ESN if the communication and CRC are valid.
 *
 * @return The ESN as a 32-bit unsigned integer. Return UINT32_MAX if an error occurred.
 */

uint32_t stx3_get_esn(void);

/**
 * @brief Resets the STX3 function.
 *
 * This function performs the reset of the STX3 function, ensuring that all
 * parameters and internal states are restored to their default values.
 *
 * @return int Returns 0 on success or a negative error code on failure.
 */

void stx3_reset(void);

/**
 * @brief Configures the STX3 module.
 *
 * This function initializes and sets up the STX3 module with the provided settings.
 *
 * @param[in] channel The channel number to configure.
 * @param[in] num_bursts The number of bursts to be configured.
 * @param[in] min_interval The minimum interval between bursts.
 * @param[in] max_interval The maximum interval between bursts.
 * @return Status code indicating success or failure of the configuration.
 */

int stx3_configure(uint8_t channel, uint8_t num_bursts, uint8_t min_interval, uint8_t max_interval);

void stx3_gpio_init(void);

#endif /* __APPS_SATELITAL_STX3_H */