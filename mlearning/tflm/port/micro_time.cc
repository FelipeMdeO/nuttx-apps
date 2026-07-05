/****************************************************************************
 * apps/mlearning/tflm/port/micro_time.cc
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <time.h>

#include "tensorflow/lite/micro/micro_time.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Tick unit: microseconds, from CLOCK_MONOTONIC.  The 32-bit tick counter
 * wraps around every ~71 minutes; MicroProfiler consumers only use tick
 * deltas, so the wraparound is harmless for profiling purposes.
 */

#define TICKS_PER_SECOND 1000000

/****************************************************************************
 * Public Functions
 ****************************************************************************/

namespace tflite
{

uint32_t ticks_per_second()
{
  return TICKS_PER_SECOND;
}

uint32_t GetCurrentTimeTicks()
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint32_t>(ts.tv_sec * TICKS_PER_SECOND +
                               ts.tv_nsec / 1000);
}

}  // namespace tflite
