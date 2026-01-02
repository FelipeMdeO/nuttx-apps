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

#include <nuttx/config.h>

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <system/smf.h>
#include "hsm_psicc2_thread.h"

#define LOG_INF(fmt, ...) do { printf("[psicc2] " fmt "\n", ##__VA_ARGS__); } while (0)
#define LOG_ERR(fmt, ...) do { printf("[psicc2][ERR] " fmt "\n", ##__VA_ARGS__); } while (0)

/* ----------------- SMF object ----------------- */
struct s_object {
  struct smf_ctx ctx;              /* MUST be first (matches SMF_CTX(obj)) */
  struct hsm_psicc2_event event;
  int foo;
};

static struct s_object s_obj;

/* Declaration of possible states */
enum demo_states {
  STATE_INITIAL,
  STATE_S,
  STATE_S1,
  STATE_S2,
  STATE_S11,
  STATE_S21,
  STATE_S211,
};

static const struct smf_state demo_states[];

/********* STATE_INITIAL *********/
static void initial_entry(void *o)
{
  LOG_INF("%s", __func__);
  struct s_object *obj = (struct s_object *)o;
  obj->foo = false;
}

static enum smf_state_result initial_run(void *o)
{
  (void)o;
  LOG_INF("%s", __func__);
  return SMF_EVENT_PROPAGATE;
}

static void initial_exit(void *o)
{
  (void)o;
  LOG_INF("%s", __func__);
}

/********* STATE_S *********/
static void s_entry(void *o) { (void)o; LOG_INF("%s", __func__); }

static enum smf_state_result s_run(void *o)
{
  LOG_INF("%s", __func__);
  struct s_object *obj = (struct s_object *)o;

  switch (obj->event.event_id) {
    case EVENT_E:
      LOG_INF("%s received EVENT_E", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
      break;
    case EVENT_I:
      if (obj->foo) {
        LOG_INF("%s received EVENT_I and set foo false", __func__);
        obj->foo = false;
      } else {
        LOG_INF("%s received EVENT_I and did nothing", __func__);
      }
      return SMF_EVENT_HANDLED;
    case EVENT_TERMINATE:
      LOG_INF("%s received EVENT_TERMINATE. Terminating", __func__);
      smf_set_terminate(SMF_CTX(obj), -1);
      break;
    default:
      break;
  }

  return SMF_EVENT_PROPAGATE;
}

static void s_exit(void *o) { (void)o; LOG_INF("%s", __func__); }

/********* STATE_S1 *********/
static void s1_entry(void *o) { (void)o; LOG_INF("%s", __func__); }

static enum smf_state_result s1_run(void *o)
{
  LOG_INF("%s", __func__);
  struct s_object *obj = (struct s_object *)o;

  switch (obj->event.event_id) {
    case EVENT_A:
      LOG_INF("%s received EVENT_A", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
      break;
    case EVENT_B:
      LOG_INF("%s received EVENT_B", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
      break;
    case EVENT_C:
      LOG_INF("%s received EVENT_C", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S2]);
      break;
    case EVENT_D:
      if (!obj->foo) {
        LOG_INF("%s received EVENT_D and acted on it", __func__);
        obj->foo = true;
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
      } else {
        LOG_INF("%s received EVENT_D and ignored it", __func__);
      }
      break;
    case EVENT_F:
      LOG_INF("%s received EVENT_F", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S211]);
      break;
    case EVENT_I:
      LOG_INF("%s received EVENT_I", __func__);
      return SMF_EVENT_HANDLED;
    default:
      break;
  }

  return SMF_EVENT_PROPAGATE;
}

static void s1_exit(void *o) { (void)o; LOG_INF("%s", __func__); }

/********* STATE_S11 *********/
static void s11_entry(void *o) { (void)o; LOG_INF("%s", __func__); }

static enum smf_state_result s11_run(void *o)
{
  LOG_INF("%s", __func__);
  struct s_object *obj = (struct s_object *)o;

  switch (obj->event.event_id) {
    case EVENT_D:
      if (obj->foo) {
        LOG_INF("%s received EVENT_D and acted upon it", __func__);
        obj->foo = false;
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
      } else {
        LOG_INF("%s received EVENT_D and ignored it", __func__);
      }
      break;
    case EVENT_G:
      LOG_INF("%s received EVENT_G", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
      break;
    case EVENT_H:
      LOG_INF("%s received EVENT_H", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
      break;
    default:
      break;
  }

  return SMF_EVENT_PROPAGATE;
}

static void s11_exit(void *o) { (void)o; LOG_INF("%s", __func__); }

/********* STATE_S2 *********/
static void s2_entry(void *o) { (void)o; LOG_INF("%s", __func__); }

static enum smf_state_result s2_run(void *o)
{
  LOG_INF("%s", __func__);
  struct s_object *obj = (struct s_object *)o;

  switch (obj->event.event_id) {
    case EVENT_C:
      LOG_INF("%s received EVENT_C", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
      break;
    case EVENT_F:
      LOG_INF("%s received EVENT_F", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
      break;
    case EVENT_I:
      if (!obj->foo) {
        LOG_INF("%s received EVENT_I and set foo true", __func__);
        obj->foo = true;
        return SMF_EVENT_HANDLED;
      } else {
        LOG_INF("%s received EVENT_I and did nothing", __func__);
      }
      break;
    default:
      break;
  }

  return SMF_EVENT_PROPAGATE;
}

static void s2_exit(void *o) { (void)o; LOG_INF("%s", __func__); }

/********* STATE_S21 *********/
static void s21_entry(void *o) { (void)o; LOG_INF("%s", __func__); }

static enum smf_state_result s21_run(void *o)
{
  LOG_INF("%s", __func__);
  struct s_object *obj = (struct s_object *)o;

  switch (obj->event.event_id) {
    case EVENT_A:
      LOG_INF("%s received EVENT_A", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
      break;
    case EVENT_B:
      LOG_INF("%s received EVENT_B", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S211]);
      break;
    case EVENT_G:
      LOG_INF("%s received EVENT_G", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
      break;
    default:
      break;
  }

  return SMF_EVENT_PROPAGATE;
}

static void s21_exit(void *o) { (void)o; LOG_INF("%s", __func__); }

/********* STATE_S211 *********/
static void s211_entry(void *o) { (void)o; LOG_INF("%s", __func__); }

static enum smf_state_result s211_run(void *o)
{
  LOG_INF("%s", __func__);
  struct s_object *obj = (struct s_object *)o;

  switch (obj->event.event_id) {
    case EVENT_D:
      LOG_INF("%s received EVENT_D", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
      break;
    case EVENT_H:
      LOG_INF("%s received EVENT_H", __func__);
      smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
      break;
    default:
      break;
  }

  return SMF_EVENT_PROPAGATE;
}

static void s211_exit(void *o) { (void)o; LOG_INF("%s", __func__); }

/* State table (same as Zephyr) */
static const struct smf_state demo_states[] = {
  [STATE_INITIAL] = SMF_CREATE_STATE(initial_entry, initial_run, initial_exit, NULL, &demo_states[STATE_S2]),
  [STATE_S]       = SMF_CREATE_STATE(s_entry,       s_run,       s_exit,       &demo_states[STATE_INITIAL], &demo_states[STATE_S11]),
  [STATE_S1]      = SMF_CREATE_STATE(s1_entry,      s1_run,      s1_exit,      &demo_states[STATE_S],       &demo_states[STATE_S11]),
  [STATE_S2]      = SMF_CREATE_STATE(s2_entry,      s2_run,      s2_exit,      &demo_states[STATE_S],       &demo_states[STATE_S211]),
  [STATE_S11]     = SMF_CREATE_STATE(s11_entry,     s11_run,     s11_exit,     &demo_states[STATE_S1],      NULL),
  [STATE_S21]     = SMF_CREATE_STATE(s21_entry,     s21_run,     s21_exit,     &demo_states[STATE_S2],      &demo_states[STATE_S211]),
  [STATE_S211]    = SMF_CREATE_STATE(s211_entry,    s211_run,    s211_exit,    &demo_states[STATE_S21],     NULL),
};

/* ----------------- NuttX plumbing ----------------- */

static int hsm_psicc2_thread_main(int argc, char **argv)
{
  (void)argc; (void)argv;

  struct mq_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.mq_maxmsg  = HSM_PSICC2_THREAD_EVENT_QUEUE_SIZE;
  attr.mq_msgsize = sizeof(struct hsm_psicc2_event);

  mqd_t mq = mq_open(HSM_PSICC2_MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);
  if (mq == (mqd_t)-1) {
    LOG_ERR("mq_open(%s) failed: %d", HSM_PSICC2_MQ_NAME, errno);
    return -1;
  }

  LOG_INF("State Machine thread started");
  smf_set_initial(SMF_CTX(&s_obj), &demo_states[STATE_INITIAL]);

  while (1) {
    ssize_t n = mq_receive(mq, (char *)&s_obj.event, sizeof(s_obj.event), NULL);
    if (n < 0) {
      LOG_ERR("mq_receive failed: %d", errno);
      continue;
    }

    int rc = smf_run_state(SMF_CTX(&s_obj));
    if (rc != 0) {
      LOG_INF("SMF terminated (rc=%d). Exiting thread", rc);
      break;
    }
  }

  mq_close(mq);
  /* opcional: mq_unlink(HSM_PSICC2_MQ_NAME); */
  return 0;
}

static bool g_started;

int hsm_psicc2_thread_start(void)
{
  if (g_started) {
    return 0;
  }

  int pid = task_create("psicc2_thread",
                        HSM_PSICC2_THREAD_PRIORITY,
                        HSM_PSICC2_THREAD_STACK_SIZE,
                        hsm_psicc2_thread_main,
                        NULL);

  if (pid < 0) {
    LOG_ERR("task_create failed: %d", errno);
    return -1;
  }

  g_started = true;
  return 0;
}

int hsm_psicc2_post_event(uint32_t event_id)
{
  struct hsm_psicc2_event ev;
  ev.event_id = event_id;

  mqd_t mq = mq_open(HSM_PSICC2_MQ_NAME, O_WRONLY | O_NONBLOCK);
  if (mq == (mqd_t)-1) {
    LOG_ERR("mq_open(O_WRONLY) failed: %d (did you run 'hsm_psicc2 start'?)", errno);
    return -1;
  }

  int rc = mq_send(mq, (const char *)&ev, sizeof(ev), 0);
  if (rc < 0) {
    LOG_ERR("mq_send failed: %d", errno);
    mq_close(mq);
    return -1;
  }

  mq_close(mq);
  return 0;
}
