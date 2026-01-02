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
#include <ctype.h>
#include <string.h>

#include "hsm_psicc2_thread.h"

static void usage(void)
{
  printf("Usage:\n");
  printf("  hsm_psicc2 start\n");
  printf("  hsm_psicc2 event <A..I>\n");
  printf("  hsm_psicc2 terminate\n");
}

int hsm_psicc2_main(int argc, char **argv)
{
  if (argc < 2) {
    usage();
    return 1;
  }

  if (!strcmp(argv[1], "start")) {
    printf("State Machine Framework Demo\n");
    printf("See PSiCC2 Fig 2.11 for the statechart\n");
    printf("https://www.state-machine.com/psicc2\n\n");
    return hsm_psicc2_thread_start();
  }

  if (!strcmp(argv[1], "terminate")) {
    return hsm_psicc2_post_event(EVENT_TERMINATE);
  }

  if (!strcmp(argv[1], "event")) {
    if (argc < 3 || argv[2] == NULL || argv[2][0] == '\0') {
      usage();
      return 1;
    }

    int c = toupper((unsigned char)argv[2][0]);
    switch (c) {
      case 'A': return hsm_psicc2_post_event(EVENT_A);
      case 'B': return hsm_psicc2_post_event(EVENT_B);
      case 'C': return hsm_psicc2_post_event(EVENT_C);
      case 'D': return hsm_psicc2_post_event(EVENT_D);
      case 'E': return hsm_psicc2_post_event(EVENT_E);
      case 'F': return hsm_psicc2_post_event(EVENT_F);
      case 'G': return hsm_psicc2_post_event(EVENT_G);
      case 'H': return hsm_psicc2_post_event(EVENT_H);
      case 'I': return hsm_psicc2_post_event(EVENT_I);
      default:
        printf("Invalid event '%c'\n", c);
        return 1;
    }
  }

  usage();
  return 1;
}

