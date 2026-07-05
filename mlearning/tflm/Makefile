############################################################################
# apps/mlearning/tflm/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

CXXEXT = .cc

TFLM_DIR = tflite-micro

# Upstream builds TFLM with these flags (tools/make/Makefile); keep them in
# sync. The kissfft *.c sources are #included by the wrappers in
# signal/src/kiss_fft_wrappers and must not be compiled separately.

CXXFLAGS += -fno-rtti -fno-exceptions -fno-threadsafe-statics
CXXFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables
CXXFLAGS += -Wno-shadow -Wno-undef

# Platform shims (port/) replace the upstream reference implementations of
# DebugLog() and micro timing; test doubles are not part of the library.

TFLM_EXCLUDE = \
	$(TFLM_DIR)/tensorflow/lite/micro/debug_log.cc \
	$(TFLM_DIR)/tensorflow/lite/micro/micro_time.cc \
	$(TFLM_DIR)/tensorflow/lite/micro/fake_micro_context.cc \
	$(TFLM_DIR)/tensorflow/lite/micro/mock_micro_graph.cc \
	$(TFLM_DIR)/tensorflow/lite/micro/test_helpers.cc \
	$(TFLM_DIR)/tensorflow/lite/micro/test_helper_custom_ops.cc

TFLM_SRCS = $(shell find $(TFLM_DIR)/tensorflow $(TFLM_DIR)/signal -name '*.cc')

# Example model data (const arrays only, no code); unused sections are
# dropped by the linker when the example is not selected. The generated
# model sources include their headers relative to the example directory.

TFLM_SRCS += $(wildcard $(TFLM_DIR)/examples/hello_world/models/*.cc)
CXXFLAGS += ${INCDIR_PREFIX}$(TFLM_DIR)/examples/hello_world

CXXSRCS = $(filter-out $(TFLM_EXCLUDE),$(TFLM_SRCS))
CXXSRCS += port/debug_log.cc
CXXSRCS += port/micro_time.cc

include $(APPDIR)/Application.mk
