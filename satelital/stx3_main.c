/****************************************************************************
 * apps/satelital/stx3_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <debug.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

#include "stx3.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Common, message formats */

const char g_stx3argrequired[]     =
           "stx3tool: %s: missing required argument(s)\n";
const char g_stx3arginvalid[]      =
           "stx3tool: %s: argument invalid\n";
const char g_stx3argrange[]        =
           "stx3tool: %s: value out of range\n";
const char g_stx3cmdnotfound[]     =
           "stx3tool: %s: command not found\n";
const char g_stx3toomanyargs[]     =
           "stx3tool: %s: too many arguments\n";
const char g_stx3cmdfailed[]       =
           "stx3tool: %s: %s failed: %d\n";
const char g_stx3xfrerror[]        =
           "stx3tool: %s: Transfer failed: %d\n";

#define MIN_BYTES 9
#define MAX_BYTES 144

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stx3_cmd_help(int argc, char **argv);
static int stx3_cmd_new_burst(int argc, char **argv);
static uint32_t stx3_cmd_get_esn(int argc, char **argv);
static int stx3_cmd_abort_burst(int argc, char **argv);
static int stx3_cmd_reset(int argc, char **argv);
static int stx3_cmd_configure(int argc, char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stx3_cmd_help
 ****************************************************************************/

static int stx3_cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Usage: ./stx3tool <option> [arguments]\n\n");
    printf("Options:\n");

    printf("  -s <byte_count> <byte1> <byte2> ... <byteN>\tExecute the 'new burst' command.\n");
    printf("      <byte_count>: Number of bytes to send (minimum %d, maximum %d).\n", MIN_BYTES, MAX_BYTES);
    printf("      <byte1> to <byteN>: Bytes to be sent, each represented by two hexadecimal digits.\n");
    printf("      Example: ./stx3tool -s 9 01 02 03 04 05 06 07 08 09\n\n");

    printf("  -e\t\t\t\t\tRetrieve the ESN.\n");
    printf("      Usage: ./stx3tool -e\n\n");

    printf("  -a\t\t\t\t\tAbort the current burst.\n");
    printf("      Usage: ./stx3tool -a\n\n");

    printf("  -r\t\t\t\t\tReset the device to factory settings.\n");
    printf("      Usage: ./stx3tool -r\n\n");

    printf("  -c <channel> <num_bursts> <min_interval> <max_interval>\tConfigure the device.\n");
    printf("      <channel>: Channel number for configuration.\n");
    printf("      <num_bursts>: Number of bursts to configure.\n");
    printf("      <min_interval>: Minimum interval between bursts.\n");
    printf("      <max_interval>: Maximum interval between bursts.\n");
    printf("      Usage: ./stx3tool -c <channel> <num_bursts> <min_interval> <max_interval>\n\n");

    return OK;
}

/****************************************************************************
 * Name: stx3_cmd_new_burst
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define SUCCESS 0
#define ERROR -1

/**
 * @brief Displays a hex dump of the given data.
 *
 * @param data Pointer to the byte array.
 * @param len Number of bytes to display.
 */

void hex_dump(const unsigned char *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

int stx3_cmd_new_burst(int byte_count, char *bytes[])
{
    int ret;

    unsigned char *byte_values = calloc(byte_count, sizeof(unsigned char));
    if (!byte_values)
    {
        printf("Memory allocation file.\n");
        return ERROR;
    }

    for (int i = 0; i < byte_count; i++)
    {
        char *endptr;
        long val = strtol(bytes[i], &endptr, 16);

        if (*endptr != '\0' || val < 0x00 || val > 0xFF)
        {
            printf("Invalid Byte %d: %s\n", i + 1, bytes[i]);
            free(byte_values);
            return ERROR;
        }

        byte_values[i] = (unsigned char)val;
        hex_dump(byte_values, byte_count);
        printf("Byte %d: 0x%02X\n", i + 1, byte_values[i]);
    }

    ret = stx3_new_burst(byte_values, byte_count);
    free(byte_values);

    return ret;
}

/****************************************************************************
 * Name: stx3_cmd_get_esn
 ****************************************************************************/

static uint32_t stx3_cmd_get_esn(int argc, char **argv)
{
    return stx3_get_esn();
}

/****************************************************************************
 * Name: stx3_cmd_abort_burst
 ****************************************************************************/

static int stx3_cmd_abort_burst(int argc, char **argv)
{
    return stx3_abort_burst();
}

/****************************************************************************
 * Name: stx3_cmd_reset
 ****************************************************************************/

static int stx3_cmd_reset(int argc, char **argv)
{
    stx3_reset();
    return OK;
}

/****************************************************************************
 * Name: stx3_cmd_configure
 ****************************************************************************/

static int stx3_cmd_configure(int argc, char **argv)
{
    uint8_t channel;
    uint8_t num_bursts;
    uint8_t min_interval;
    uint8_t max_interval;

    channel = atoi(argv[0]);
    num_bursts = atoi(argv[1]);
    min_interval = atoi(argv[2]);
    max_interval = atoi(argv[3]);

    return stx3_configure(channel, num_bursts, min_interval, max_interval);
}

/****************************************************************************
 * Name: stx3_execute
 ****************************************************************************/

static int stx3_execute(int argc, char *argv[])
{
    if (argc < 2)
    {
        return stx3_cmd_help(argc, argv);
    }

    if (argv[1][0] != '-' || argv[1][1] == '\0')
    {
        return stx3_cmd_help(argc, argv);
    }

    switch (argv[1][1])
    {
        case 's':
        {
            if (argc < 3)
            {
                printf(g_stx3argrequired, "new_burst");
                return ERROR;
            }

            char *endptr;
            long byte_count = strtol(argv[2], &endptr, 10);

            if (*endptr != '\0')
            {
                printf("Invalid byte count: %s. It must be an integer between %d and %d.\n", argv[2], MIN_BYTES, MAX_BYTES);
                return ERROR;
            }

            if (byte_count < MIN_BYTES || byte_count > MAX_BYTES)
            {
                printf("Number of bytes outside the permitted range: %ld. Must be between %d and %d.\n", byte_count, MIN_BYTES, MAX_BYTES);
                return ERROR;
            }

            int expected_argc = 3 + (int)byte_count; // ./stx3tool -s <byte_count> <bytes...>

            if (argc != expected_argc)
            {
                printf(g_stx3arginvalid, "new_burst");
                printf("Right Use: ./stx3tool -s %ld <byte1> <byte2> ... <byte%ld>\n", byte_count, byte_count);
                return ERROR;
            }

            // Valida cada byte fornecido
            for (int i = 3; i < argc; i++)
            {
                if (strlen(argv[i]) != 2 || 
                    !isxdigit(argv[i][0]) || 
                    !isxdigit(argv[i][1]))
                {
                    printf("Invalid byte at position %d: %s. Each byte must be represented by two hexadecimal digits.\n", i - 2, argv[i]);
                    return ERROR;
                }
            }

            return stx3_cmd_new_burst((int)byte_count, &argv[3]);
        }
        case 'e':
            if (argc != 2)
            {
                printf(g_stx3argrequired, "get_esn");
                return ERROR;
            }
            return stx3_cmd_get_esn(argc - 1, &argv[1]);
        case 'a':
            if (argc != 2)
            {
                printf(g_stx3argrequired, "abort_burst");
                return ERROR;
            }
            return stx3_cmd_abort_burst(argc - 1, &argv[1]);
        case 'r':
            if (argc != 2)
            {
                printf(g_stx3argrequired, "reset");
                return ERROR;
            }
            return stx3_cmd_reset(argc - 1, &argv[1]);
        case 'c':
            if (argc != 6)
            {
                printf(g_stx3argrequired, "configure");
                return ERROR;
            }
            return stx3_cmd_configure(argc - 2, &argv[2]);
        default:
            return stx3_cmd_help(argc, argv);
    }
}

int main(int argc, char *argv[])
{  
    return stx3_execute(argc, argv);
}
