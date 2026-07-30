/****************************************************************************
 * apps/examples/sensor_fusion/sensor_fusion_main.c
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
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sensor/accel.h>
#include <sensor/gyro.h>

#include "Fusion/Fusion.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SENSOR_FUSION_PERIOD_US \
  (CONFIG_EXAMPLES_SENSOR_FUSION_SAMPLE_RATE * 1000)

#define SENSOR_FUSION_RATE_HZ \
  (1000.0f / CONFIG_EXAMPLES_SENSOR_FUSION_SAMPLE_RATE)

/* The uORB topics carry SI units, the Fusion library expects degrees per
 * second and g.  See FusionAhrsUpdateNoMagnetometer() in FusionAhrs.c.
 */

#define RADPS_TO_DPS 57.29577951f
#define MPS2_TO_G    (1.0f / 9.80665f)

/* Settings for a 6-axis IMU without magnetometer.  The gyroscope range
 * matches what the MPU6050 driver programs into the device; a wider range
 * only makes the clipping detection less sensitive.
 */

#define SENSOR_FUSION_GAIN            0.5f
#define SENSOR_FUSION_GYRO_RANGE      250.0f
#define SENSOR_FUSION_ACCEL_REJECTION 10.0f
#define SENSOR_FUSION_TIMEOUT         5.0f

/* A driver may sleep the device when the last subscriber goes away, as the
 * MPU6050 one does, so a run can start on a device that has just left sleep
 * mode.  Let it settle before feeding the filter, or the first samples drag
 * the estimate away.
 */

#define SENSOR_FUSION_SETTLE_US 100000

/* orb_copy() returns -1 both for a failed read and for a short one, so a
 * sample is retried a few times instead of being fatal on the first miss.
 */

#define SENSOR_FUSION_MAX_RETRIES 5

#define SENSOR_FUSION_ACCEL   0
#define SENSOR_FUSION_GYRO    1
#define SENSOR_FUSION_NTOPICS 2

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sensor_fusion_subscribe
 *
 * Description:
 *   Subscribe to one topic of instance 0 and make the descriptor
 *   non-blocking.
 *
 *   A sensor whose lower half only provides fetch() never pushes samples
 *   into the circular buffer, and sensor_read() only reads such a device
 *   when the descriptor is non-blocking.  A blocking descriptor would wait
 *   forever instead.
 *
 ****************************************************************************/

static int sensor_fusion_subscribe(FAR const struct orb_metadata *meta)
{
  int flags;
  int fd;

  fd = orb_subscribe_multi(meta, 0);
  if (fd < 0)
    {
      fprintf(stderr, "Failed to subscribe to %s: %d\n",
              meta->o_name, errno);
      return fd;
    }

  flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      fprintf(stderr, "Failed to set %s non-blocking: %d\n",
              meta->o_name, errno);
      orb_unsubscribe(fd);
      return ERROR;
    }

  return fd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * sensor_fusion_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const struct orb_metadata *meta[SENSOR_FUSION_NTOPICS];
  int fd[SENSOR_FUSION_NTOPICS];
  struct sensor_accel accel;
  struct sensor_gyro gyro;
  FusionAhrs ahrs;
  FusionAhrsSettings settings;
  FusionVector accelerometer;
  FusionVector gyroscope;
  FusionEuler euler;
  int iterations = CONFIG_EXAMPLES_SENSOR_FUSION_SAMPLES;
  int ret = OK;
  int retries;
  int i;

  if (argc == 2)
    {
      iterations = atoi(argv[1]);
    }
  else if (argc > 2)
    {
      printf("Usage: %s [<samples>]\n", argv[0]);
      printf("  Fuse the accelerometer and gyroscope uORB topics into\n");
      printf("  yaw, pitch and roll.  <samples> defaults to %d; 0 runs\n",
             CONFIG_EXAMPLES_SENSOR_FUSION_SAMPLES);
      printf("  until the task is killed.\n");
      return EXIT_FAILURE;
    }

  meta[SENSOR_FUSION_ACCEL] = ORB_ID(sensor_accel);
  meta[SENSOR_FUSION_GYRO]  = ORB_ID(sensor_gyro);

  for (i = 0; i < SENSOR_FUSION_NTOPICS; i++)
    {
      fd[i] = sensor_fusion_subscribe(meta[i]);
      if (fd[i] < 0)
        {
          while (--i >= 0)
            {
              orb_unsubscribe(fd[i]);
            }

          return EXIT_FAILURE;
        }
    }

  FusionAhrsInitialise(&ahrs);

  settings.sampleRate            = SENSOR_FUSION_RATE_HZ;
  settings.convention            = FusionConventionNwu;
  settings.gain                  = SENSOR_FUSION_GAIN;
  settings.gyroscopeRange        = SENSOR_FUSION_GYRO_RANGE;
  settings.accelerationRejection = SENSOR_FUSION_ACCEL_REJECTION;
  settings.magneticRejection     = 0.0f;
  settings.rejectionTimeout      = SENSOR_FUSION_TIMEOUT;

  FusionAhrsSetSettings(&ahrs, &settings);

  printf("Sensor Fusion example\n");
  printf("Sample Rate: %.2f Hz\n", SENSOR_FUSION_RATE_HZ);

  usleep(SENSOR_FUSION_SETTLE_US);

  for (i = 0, retries = 0; iterations == 0 || i < iterations; )
    {
      if (orb_copy(meta[SENSOR_FUSION_ACCEL],
                   fd[SENSOR_FUSION_ACCEL], &accel) < 0 ||
          orb_copy(meta[SENSOR_FUSION_GYRO],
                   fd[SENSOR_FUSION_GYRO], &gyro) < 0)
        {
          if (++retries > SENSOR_FUSION_MAX_RETRIES)
            {
              fprintf(stderr, "Giving up after %d failed samples\n",
                      retries);
              ret = ERROR;
              break;
            }

          usleep(SENSOR_FUSION_PERIOD_US);
          continue;
        }

      retries = 0;
      i++;

      accelerometer.axis.x = accel.x * MPS2_TO_G;
      accelerometer.axis.y = accel.y * MPS2_TO_G;
      accelerometer.axis.z = accel.z * MPS2_TO_G;

      gyroscope.axis.x = gyro.x * RADPS_TO_DPS;
      gyroscope.axis.y = gyro.y * RADPS_TO_DPS;
      gyroscope.axis.z = gyro.z * RADPS_TO_DPS;

      FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer);
      euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));

      printf("Yaw: %.3f | Pitch: %.3f | Roll: %.3f\n",
             euler.angle.yaw, euler.angle.pitch, euler.angle.roll);

      usleep(SENSOR_FUSION_PERIOD_US);
    }

  for (i = 0; i < SENSOR_FUSION_NTOPICS; i++)
    {
      orb_unsubscribe(fd[i]);
    }

  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
