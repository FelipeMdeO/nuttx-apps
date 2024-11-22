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
#ifdef __NuttX_
#include <nuttx/config.h>
#include <debug.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

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

#define OK 0
#define ERROR -1


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
    printf("Usage stx3 <cmd> [arguments]:                                   \n");
    printf("  stx3 [-s] <9 bytes hex data>  - Execute new burst command     \n");
    printf("  stx3 [-e]                     - Get ESN                       \n");
    printf("  stx3 [-a]                     - Abort current burst           \n");
    printf("  stx3 [-r]                     - Recovery to factory config    \n");
    printf("  stx3 [-c] <channel> <num_burts> <min_internval> <max interval>\n");
    return OK;
}

/****************************************************************************
 * Name: stx3_cmd_new_burst
 ****************************************************************************/

static int stx3_cmd_new_burst(int argc, char **argv)
{
    uint8_t data[9];
    int i;

    /* 9 bytes hex = 18 characters */

    if (argc != 9 )
    {
        printf(g_stx3arginvalid, "new_burst");
        return ERROR;
    }

    for (i = 0; i < 9; i++) 
    {
        // Converta cada string para uint8_t e armazene em data
        if (sscanf(argv[i], "%2hhx", &data[i]) != 1) 
        {
            printf(g_stx3arginvalid, "new_burst");
            return ERROR;
        }
    }

    // Call the function to handle new burst with the provided hex data
    return stx3_new_burst(data, 9U);
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
            if (argc != 11)
            {
                printf(g_stx3arginvalid, "new_burst");
                return ERROR;
            }
            return stx3_cmd_new_burst(argc - 2, &argv[2]);
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
