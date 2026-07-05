/****************************************************************************
 * apps/mlearning/tflm/port/debug_log.cc
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

#include <cstdarg>
#include <cstdio>

#include <syslog.h>

#include "tensorflow/lite/micro/debug_log.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C" void DebugLog(const char *format, va_list args)
{
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  vsyslog(LOG_INFO, format, args);
#endif
}

#ifndef TF_LITE_STRIP_ERROR_STRINGS
extern "C" int DebugVsnprintf(char *buffer, size_t buf_size,
                              const char *format, va_list vlist)
{
  return vsnprintf(buffer, buf_size, format, vlist);
}
#endif
