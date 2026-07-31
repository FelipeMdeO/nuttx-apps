/****************************************************************************
 * apps/examples/imu_logger/imu_logger_main.c
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/input/buttons.h>

#include <sensor/accel.h>
#include <sensor/gyro.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IMU_LOGGER_PERIOD_US (1000000 / CONFIG_EXAMPLES_IMU_LOGGER_FREQUENCY)

/* The driver sleeps the device (PWR_MGMT_1) when the last subscriber goes
 * away and wakes it when the first one arrives, so a run always starts on a
 * device that has just left sleep mode.  Give it time to settle before the
 * first sample, or the gyroscope reports a large transient.
 */

#define IMU_LOGGER_SETTLE_US 100000

#define IMU_LOGGER_ACCEL   0
#define IMU_LOGGER_GYRO    1
#define IMU_LOGGER_NTOPICS 2

#define IMU_LOGGER_CSV_HEADER "accX;accY;accZ;gyroX;gyroY;gyroZ;timestamp\n"
#define IMU_LOGGER_CSV_ROW    "%.6f;%.6f;%.6f;%.6f;%.6f;%.6f;%" PRIu64 "\n"

/* ws2812 pixels are 0x00RRGGBB */

#define IMU_LOGGER_LED_RECORDING 0x0000ff00
#define IMU_LOGGER_LED_IDLE      0x00ff0000

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: imu_logger_led_open
 *
 * Description:
 *   Open the status LED device, if one is configured.  Its failure modes
 *   are non-fatal: a missing or misbehaving LED should not stop logging.
 *
 ****************************************************************************/

static int imu_logger_led_open(void)
{
  if (strlen(CONFIG_EXAMPLES_IMU_LOGGER_LED_DEVPATH) == 0)
    {
      return -1;
    }

  return open(CONFIG_EXAMPLES_IMU_LOGGER_LED_DEVPATH, O_WRONLY);
}

/****************************************************************************
 * Name: imu_logger_led_set
 ****************************************************************************/

static void imu_logger_led_set(int led_fd, uint32_t color)
{
  if (led_fd < 0)
    {
      return;
    }

  lseek(led_fd, 0, SEEK_SET);
  write(led_fd, &color, sizeof(color));
}

/****************************************************************************
 * Name: imu_logger_subscribe
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

static int imu_logger_subscribe(FAR const struct orb_metadata *meta)
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
 * Name: imu_logger_next_index
 *
 * Description:
 *   Scan CONFIG_EXAMPLES_IMU_LOGGER_MOUNTPOINT for "imu_<n>.csv" files left
 *   over from previous sessions and return one past the highest index
 *   found, so a run never overwrites a previous recording.
 *
 ****************************************************************************/

static unsigned int imu_logger_next_index(void)
{
  FAR DIR *dir;
  FAR struct dirent *entry;
  unsigned int next = 0;
  unsigned int found;

  dir = opendir(CONFIG_EXAMPLES_IMU_LOGGER_MOUNTPOINT);
  if (dir == NULL)
    {
      return 0;
    }

  while ((entry = readdir(dir)) != NULL)
    {
      if (sscanf(entry->d_name, "imu_%u.csv", &found) == 1 &&
          found + 1 > next)
        {
          next = found + 1;
        }
    }

  closedir(dir);
  return next;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * imu_logger_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const struct orb_metadata *meta[IMU_LOGGER_NTOPICS];
  int fd[IMU_LOGGER_NTOPICS];
  struct sensor_accel accel;
  struct sensor_gyro gyro;
  char path[64];
  FAR FILE *file = NULL;
  unsigned int index;
  unsigned long session_samples = 0;
  btn_buttonset_t buttons;
  btn_buttonset_t prev_buttons = 0;
  int btn_fd;
  int led_fd;
  int i;

  btn_fd = open(CONFIG_EXAMPLES_IMU_LOGGER_BUTTON_DEVPATH, O_RDONLY);
  if (btn_fd < 0)
    {
      fprintf(stderr, "Failed to open %s: %d\n",
              CONFIG_EXAMPLES_IMU_LOGGER_BUTTON_DEVPATH, errno);
      return EXIT_FAILURE;
    }

  led_fd = imu_logger_led_open();

  meta[IMU_LOGGER_ACCEL] = ORB_ID(sensor_accel);
  meta[IMU_LOGGER_GYRO]  = ORB_ID(sensor_gyro);

  for (i = 0; i < IMU_LOGGER_NTOPICS; i++)
    {
      fd[i] = imu_logger_subscribe(meta[i]);
      if (fd[i] < 0)
        {
          while (--i >= 0)
            {
              orb_unsubscribe(fd[i]);
            }

          close(btn_fd);
          return EXIT_FAILURE;
        }
    }

  index = imu_logger_next_index();
  imu_logger_led_set(led_fd, IMU_LOGGER_LED_IDLE);

  printf("imu_logger: %d Hz, press the button to start or stop "
         "recording\n", CONFIG_EXAMPLES_IMU_LOGGER_FREQUENCY);

  usleep(IMU_LOGGER_SETTLE_US);

  for (; ; )
    {
      orb_abstime start = orb_absolute_time();

      if (read(btn_fd, &buttons, sizeof(buttons)) == sizeof(buttons))
        {
          bool pressed = (buttons & CONFIG_EXAMPLES_IMU_LOGGER_BUTTON_MASK)
                         != 0;
          bool was_pressed = (prev_buttons &
                              CONFIG_EXAMPLES_IMU_LOGGER_BUTTON_MASK) != 0;

          prev_buttons = buttons;

          if (pressed && !was_pressed)
            {
              if (file == NULL)
                {
                  snprintf(path, sizeof(path), "%s/imu_%03u.csv",
                           CONFIG_EXAMPLES_IMU_LOGGER_MOUNTPOINT, index);

                  file = fopen(path, "w");
                  if (file == NULL)
                    {
                      fprintf(stderr, "Failed to create %s: %d\n",
                              path, errno);
                      continue;
                    }

                  fputs(IMU_LOGGER_CSV_HEADER, file);
                  session_samples = 0;
                  imu_logger_led_set(led_fd, IMU_LOGGER_LED_RECORDING);
                  printf("Recording started: %s\n", path);
                }
              else
                {
                  fclose(file);
                  file = NULL;
                  imu_logger_led_set(led_fd, IMU_LOGGER_LED_IDLE);
                  printf("Recording stopped: %s (%lu samples)\n",
                         path, session_samples);
                  index++;
                }
            }
        }

      if (file != NULL)
        {
          if (orb_copy(meta[IMU_LOGGER_ACCEL], fd[IMU_LOGGER_ACCEL],
                       &accel) >= 0 &&
              orb_copy(meta[IMU_LOGGER_GYRO], fd[IMU_LOGGER_GYRO],
                       &gyro) >= 0)
            {
              fprintf(file, IMU_LOGGER_CSV_ROW,
                      accel.x, accel.y, accel.z,
                      gyro.x, gyro.y, gyro.z,
                      (uint64_t)start);
              fflush(file);
              session_samples++;
            }
        }

      orb_abstime elapsed = orb_absolute_time() - start;
      if (elapsed < (orb_abstime)IMU_LOGGER_PERIOD_US)
        {
          usleep((unsigned)((orb_abstime)IMU_LOGGER_PERIOD_US - elapsed));
        }
    }

  for (i = 0; i < IMU_LOGGER_NTOPICS; i++)
    {
      orb_unsubscribe(fd[i]);
    }

  if (led_fd >= 0)
    {
      close(led_fd);
    }

  close(btn_fd);
  return EXIT_SUCCESS;
}
