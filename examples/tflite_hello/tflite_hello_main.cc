/****************************************************************************
 * apps/examples/tflite_hello/tflite_hello_main.cc
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

/* Port of the upstream tflite-micro hello_world example (sine model).
 * Prints each prediction both as a decimal and as raw IEEE-754 bits so
 * runs on different architectures can be compared bit-exactly.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "examples/hello_world/models/hello_world_float_model_data.h"
#include "examples/hello_world/models/hello_world_int8_model_data.h"
#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

namespace
{

using HelloWorldOpResolver = tflite::MicroMutableOpResolver<1>;

/* The tensor arena must be separate from the task stack. */

alignas(16) uint8_t g_tensor_arena[CONFIG_TFLITE_MICRO_ARENA_SIZE];

constexpr int kNumTestValues = 4;
constexpr float kEpsilon = 0.05f;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

uint32_t float_bits(float value)
{
  uint32_t bits;

  memcpy(&bits, &value, sizeof(bits));
  return bits;
}

TfLiteStatus register_ops(HelloWorldOpResolver &op_resolver)
{
  TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
  return kTfLiteOk;
}

TfLiteStatus run_float_model(int *failures)
{
  const tflite::Model *model =
      tflite::GetModel(g_hello_world_float_model_data);

  if (model->version() != TFLITE_SCHEMA_VERSION)
    {
      printf("schema version mismatch: model %" PRIu32 " expected %d\n",
             model->version(), TFLITE_SCHEMA_VERSION);
      return kTfLiteError;
    }

  HelloWorldOpResolver op_resolver;
  TF_LITE_ENSURE_STATUS(register_ops(op_resolver));

  tflite::MicroInterpreter interpreter(model, op_resolver, g_tensor_arena,
                                       sizeof(g_tensor_arena));
  TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());

  const float inputs[kNumTestValues] =
    {
      0.0f, 1.0f, 3.0f, 5.0f
    };

  printf("float model:\n");
  for (int i = 0; i < kNumTestValues; i++)
    {
      interpreter.input(0)->data.f[0] = inputs[i];
      TF_LITE_ENSURE_STATUS(interpreter.Invoke());

      float y_pred = interpreter.output(0)->data.f[0];
      float y_true = sinf(inputs[i]);

      printf("  x=%.6f y_pred=%.6f (bits=0x%08" PRIx32 ") sin(x)=%.6f\n",
             inputs[i], y_pred, float_bits(y_pred), y_true);

      if (fabsf(y_true - y_pred) > kEpsilon)
        {
          (*failures)++;
        }
    }

  printf("  arena_used_bytes=%zu\n", interpreter.arena_used_bytes());
  return kTfLiteOk;
}

TfLiteStatus run_int8_model(int *failures)
{
  const tflite::Model *model =
      tflite::GetModel(g_hello_world_int8_model_data);

  if (model->version() != TFLITE_SCHEMA_VERSION)
    {
      printf("schema version mismatch: model %" PRIu32 " expected %d\n",
             model->version(), TFLITE_SCHEMA_VERSION);
      return kTfLiteError;
    }

  HelloWorldOpResolver op_resolver;
  TF_LITE_ENSURE_STATUS(register_ops(op_resolver));

  tflite::MicroInterpreter interpreter(model, op_resolver, g_tensor_arena,
                                       sizeof(g_tensor_arena));
  TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());

  TfLiteTensor *input = interpreter.input(0);
  TfLiteTensor *output = interpreter.output(0);

  const float inputs_float[kNumTestValues] =
    {
      0.77f, 1.57f, 2.3f, 3.14f
    };

  /* Quantized as (x / input_scale) + input_zero_point, precomputed by the
   * upstream test for the published int8 model.
   */

  const int8_t inputs_int8[kNumTestValues] =
    {
      -96, -63, -34, 0
    };

  printf("int8 model:\n");
  for (int i = 0; i < kNumTestValues; i++)
    {
      input->data.int8[0] = inputs_int8[i];
      TF_LITE_ENSURE_STATUS(interpreter.Invoke());

      int8_t raw = output->data.int8[0];
      float y_pred =
          (raw - output->params.zero_point) * output->params.scale;
      float y_true = sinf(inputs_float[i]);

      printf("  x=%.6f y_pred=%.6f (raw=%d) sin(x)=%.6f\n",
             inputs_float[i], y_pred, raw, y_true);

      if (fabsf(y_true - y_pred) > kEpsilon)
        {
          (*failures)++;
        }
    }

  printf("  arena_used_bytes=%zu\n", interpreter.arena_used_bytes());
  return kTfLiteOk;
}

}  // namespace

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C" int main(int argc, char *argv[])
{
  int failures = 0;

  tflite::InitializeTarget();

  if (run_float_model(&failures) != kTfLiteOk ||
      run_int8_model(&failures) != kTfLiteOk)
    {
      printf("tflite_hello: FAILED (interpreter error)\n");
      return 1;
    }

  if (failures > 0)
    {
      printf("tflite_hello: FAILED (%d predictions out of tolerance)\n",
             failures);
      return 1;
    }

  printf("tflite_hello: ALL TESTS PASSED\n");
  return 0;
}
