############################################################################
# apps/examples/sensor_fusion/Makefile
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

# Fusion, the MIT licensed AHRS library from x-io Technologies, is fetched
# at build time rather than vendored here.

FUSION_VERSION = 1.3.2
FUSION_URL     = https://github.com/xioTechnologies/Fusion/archive/refs/tags
FUSION_ZIP     = fusion-$(FUSION_VERSION).zip
FUSION_DIR     = fusion

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/examples/sensor_fusion/$(FUSION_DIR)

CSRCS += $(FUSION_DIR)/Fusion/FusionAhrs.c
CSRCS += $(FUSION_DIR)/Fusion/FusionBias.c
CSRCS += $(FUSION_DIR)/Fusion/FusionCompass.c

PROGNAME = $(CONFIG_EXAMPLES_SENSOR_FUSION_PROGNAME)
PRIORITY = $(CONFIG_EXAMPLES_SENSOR_FUSION_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_SENSOR_FUSION_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_SENSOR_FUSION)

MAINSRC = sensor_fusion_main.c

$(FUSION_ZIP):
	$(Q) curl -sSL $(FUSION_URL)/v$(FUSION_VERSION).zip -o $(FUSION_ZIP)
	$(Q) unzip -q -o $(FUSION_ZIP)
	$(Q) mv Fusion-$(FUSION_VERSION) $(FUSION_DIR)

context:: $(FUSION_ZIP)

distclean::
	$(call DELDIR, $(FUSION_DIR))
	$(call DELFILE, $(FUSION_ZIP))

include $(APPDIR)/Application.mk
