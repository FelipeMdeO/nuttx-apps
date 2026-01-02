/*
 * Copyright (c) 2024 Glenn Andrews
 * State Machine example copyright (c) Miro Samek
 *
 * Implementation of the statechart in Figure 2.11 of
 * Practical UML Statecharts in C/C++, 2nd Edition by Miro Samek
 * https://www.state-machine.com/psicc2
 * Used with permission of the author.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HSM_PSICC2_THREAD_H
#define _HSM_PSICC2_THREAD_H

#include <stdint.h>

#define HSM_PSICC2_THREAD_STACK_SIZE       2048
#define HSM_PSICC2_THREAD_PRIORITY         120
#define HSM_PSICC2_THREAD_EVENT_QUEUE_SIZE 10

#define HSM_PSICC2_MQ_NAME "/hsm_psicc2"

struct hsm_psicc2_event {
  uint32_t event_id;
};

enum demo_events {
  EVENT_A,
  EVENT_B,
  EVENT_C,
  EVENT_D,
  EVENT_E,
  EVENT_F,
  EVENT_G,
  EVENT_H,
  EVENT_I,
  EVENT_TERMINATE,
};

int hsm_psicc2_thread_start(void);
int hsm_psicc2_post_event(uint32_t event_id);

#endif
