/****************************************************************************
 * apps/examples/mpu6050/mpu6050_uorb_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sensor/accel.h>
#include <sensor/gyro.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MPU6050_PERIOD_US (1000000 / CONFIG_EXAMPLES_MPU6050_FREQUENCY)

/* The driver sleeps the device (PWR_MGMT_1) when the last subscriber goes
 * away and wakes it when the first one arrives, so a run always starts on a
 * device that has just left sleep mode.  Give it time to settle before the
 * first sample, or the gyroscope reports a large transient.
 */

#define MPU6050_SETTLE_US 100000

/* orb_copy() returns -1 both for a failed read and for a short one, and
 * only the former sets errno, so a sample is retried a few times before
 * giving up rather than treated as fatal on the first miss.
 */

#define MPU6050_MAX_RETRIES 5

#define MPU6050_ACCEL   0
#define MPU6050_GYRO    1
#define MPU6050_NTOPICS 2

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mpu6050_usage
 ****************************************************************************/

static void mpu6050_usage(FAR const char *progname)
{
  printf("Usage: %s [<samples>]\n", progname);
  printf("  Print accelerometer and gyroscope samples from the MPU6050\n");
  printf("  uORB topics.  <samples> defaults to %d; 0 runs until the task\n",
         CONFIG_EXAMPLES_MPU6050_SAMPLES);
  printf("  is killed.\n");
}

/****************************************************************************
 * Name: mpu6050_subscribe
 *
 * Description:
 *   Subscribe to one topic of instance 0 and make the descriptor
 *   non-blocking.
 *
 *   The MPU6050 lower half only provides fetch(): it has no interrupt
 *   support, so nothing ever pushes a sample into the circular buffer.  On
 *   such a sensor the upper half only reads the device from read() when the
 *   descriptor is non-blocking (see sensor_read() in drivers/sensors/
 *   sensor.c); a blocking descriptor would instead wait forever for a
 *   sample that is never pushed.  For the same reason poll() is of no use
 *   here, and the sampling period is kept by the caller.
 *
 ****************************************************************************/

static int mpu6050_subscribe(FAR const struct orb_metadata *meta)
{
  int flags;
  int fd;
  int ret;

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

  /* Ask for the sampling frequency.  The MPU6050 driver does not implement
   * set_interval, so -ENOTSUP is the expected answer and is not an error:
   * the loop below paces itself.
   */

  ret = orb_set_frequency(fd, CONFIG_EXAMPLES_MPU6050_FREQUENCY);
  if (ret < 0 && errno != ENOTSUP)
    {
      fprintf(stderr, "Failed to set %s frequency: %d\n",
              meta->o_name, errno);
    }

  return fd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * mpu6050_uorb_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const struct orb_metadata *meta[MPU6050_NTOPICS];
  int fd[MPU6050_NTOPICS];
  struct sensor_accel accel;
  struct sensor_gyro gyro;
  int samples = CONFIG_EXAMPLES_MPU6050_SAMPLES;
  int ret = OK;
  int retries;
  int count;
  int i;

  if (argc > 2)
    {
      mpu6050_usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (argc == 2)
    {
      samples = atoi(argv[1]);
      if (samples < 0)
        {
          mpu6050_usage(argv[0]);
          return EXIT_FAILURE;
        }
    }

  meta[MPU6050_ACCEL] = ORB_ID(sensor_accel);
  meta[MPU6050_GYRO]  = ORB_ID(sensor_gyro);

  for (i = 0; i < MPU6050_NTOPICS; i++)
    {
      fd[i] = mpu6050_subscribe(meta[i]);
      if (fd[i] < 0)
        {
          while (--i >= 0)
            {
              orb_unsubscribe(fd[i]);
            }

          return EXIT_FAILURE;
        }
    }

  printf("MPU6050 uORB example: %d Hz, ",
         CONFIG_EXAMPLES_MPU6050_FREQUENCY);
  if (samples > 0)
    {
      printf("%d samples\n", samples);
    }
  else
    {
      printf("until killed\n");
    }

  usleep(MPU6050_SETTLE_US);

  for (count = 0, retries = 0; samples == 0 || count < samples; )
    {
      if (orb_copy(meta[MPU6050_ACCEL], fd[MPU6050_ACCEL], &accel) < 0 ||
          orb_copy(meta[MPU6050_GYRO], fd[MPU6050_GYRO], &gyro) < 0)
        {
          if (++retries > MPU6050_MAX_RETRIES)
            {
              fprintf(stderr, "Giving up after %d failed samples\n",
                      retries);
              ret = ERROR;
              break;
            }

          usleep(MPU6050_PERIOD_US);
          continue;
        }

      retries = 0;
      count++;

      /* The sensor_accel topic is in m/s^2 and sensor_gyro in rad/s.  Both
       * carry the temperature reported by the device, in degrees Celsius.
       */

      printf("accel [m/s^2] %8.3f %8.3f %8.3f | "
             "gyro [rad/s] %7.3f %7.3f %7.3f | %5.2f C\n",
             accel.x, accel.y, accel.z,
             gyro.x, gyro.y, gyro.z,
             accel.temperature);

      usleep(MPU6050_PERIOD_US);
    }

  for (i = 0; i < MPU6050_NTOPICS; i++)
    {
      orb_unsubscribe(fd[i]);
    }

  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
