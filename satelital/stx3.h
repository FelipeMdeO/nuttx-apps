
/****************************************************************************
 * apps/include/industry/stx3.h
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

#ifndef __APPS_INCLUDE_INDUSTRY_STX3_H
#define __APPS_INCLUDE_INDUSTRY_STX3_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Define your macros here */

#define STX3_EXAMPLE_MASK 0xFF

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum {
    BURST_RUNNING,
    BURST_AVAILABLE,
    BURST_ERROR
} STX3_BURST_STATES;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initializes the STX3 module.
 *
 * This function sets up the necessary configurations and prepares the STX3
 * module for operation. It must be called before any other STX3 functions
 * are utilized.
 *
 * @return true if the initialization was successful, false otherwise.
 */

bool stx3_init(void);

/**
 * @brief Resets the STX3 module.
 *
 * This function performs a reset of the STX3 module, returning it to its initial
 * default state. It should be called to recover from errors or to reinitialize the
 * module during operation.
 */

void stx3_reset(void);


bool stx3_get_init_status(void);
/**
 * @brief Retrieves the current STX3 burst state.
 *
 * @return The current state of type STX3_BURST_STATES.
 */

STX3_BURST_STATES stx3_get_state(void);

/**
 * @brief Initiates a new burst of transmission with the given payload.
 * 
 * This function create a command for initiating a new transmission, 
 * calculates the CRC for the command, and sends it via UART. It verifies 
 * the response to determine if the burst transmission was successfully started.
 *
 * @param payload_ptr Pointer to the payload data to be included in the burst command.
 * @param payload_len Length of the payload data in bytes.
 * @return The state of the burst transmission after attempting to send the command.
 *         Possible return values are:
 *         - BURST_RUNNING: The burst cycle is ongoing.
 *         - BURST_AVAILABLE: Available for new burst.
 *         - BURST_ERROR: An error occurred during the burst transmission setup.
*/
STX3_BURST_STATES stx3_new_burst(
                                    const uint8_t *payload_ptr,
                                    uint8_t payload_len,
                                    uint32_t *buffer_error_msg,
                                    uint32_t buffer_error_size
                                );

/**
 * @brief Get the current state of the burst transmission.
 * 
 * This function sends a command to query the state of the burst transmission via UART,
 * and interprets the response to determine if the burst transmission is running,
 * available, or if an error occurred.
 *
 * @return The current state of the burst transmission.
 *         Possible return values are:
 *         - BURST_RUNNING: The burst transmission is currently running.
 *         - BURST_AVAILABLE: The burst transmission is available.
 *         - BURST_ERROR: An error occurred while querying the burst state.
 */
STX3_BURST_STATES stx3_get_burst_state(void);

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
STX3_BURST_STATES stx3_abort_burst(void);

/**
 * @brief Retrieve the ESN (Electronic Serial Number) from the STX3 module.
 * 
 * This function sends a command to request the ESN from the STX3 module via UART,
 * and interprets the response to extract the ESN if the communication and CRC are valid.
 *
 * @return The ESN as a 32-bit unsigned integer. Returns UINT32_MAX if an error occurred.
 */
uint32_t stx3_get_esn(void);

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_INDUSTRY_STX3_H */