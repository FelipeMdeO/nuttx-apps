#!/usr/bin/env bash
############################################################################
# apps/mlearning/tflm/scripts/sync_from_tflite_micro.sh
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

# Regenerates the tflite-micro/ source tree from an upstream tflite-micro checkout,
# using the same project-generation script that the Espressif and Zephyr
# ports rely on. Requires python3 with numpy and pillow available.
#
# Usage: sync_from_tflite_micro.sh <path-to-tflite-micro-checkout>

set -euo pipefail

UPSTREAM=${1:?usage: $0 <path-to-tflite-micro-checkout>}
PKGDIR=$(cd "$(dirname "$0")/.." && pwd)
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

cd "$UPSTREAM"
python3 tensorflow/lite/micro/tools/project_generation/create_tflm_tree.py \
  "$TMPDIR/tflite-micro" --examples hello_world

GIT_REV=$(git -C "$UPSTREAM" rev-parse HEAD 2>/dev/null || echo unknown)

rm -rf "$PKGDIR/tflite-micro"
mv "$TMPDIR/tflite-micro" "$PKGDIR/tflite-micro"
cp "$UPSTREAM/LICENSE" "$PKGDIR/tflite-micro/LICENSE"
echo "$GIT_REV" > "$PKGDIR/tflite-micro/UPSTREAM_REVISION"

echo "Synced tflite-micro/ from $UPSTREAM at revision $GIT_REV"
