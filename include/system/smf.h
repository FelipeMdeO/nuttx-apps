/****************************************************************************
 * apps/include/smf.h
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

#ifndef __APPS_INCLUDE_SMF_SMF_H
#define __APPS_INCLUDE_SMF_SMF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Macro to create a hierarchical state with initial transitions.
 *
 * @param _entry   State entry function or NULL
 * @param _run     State run function or NULL
 * @param _exit    State exit function or NULL
 * @param _parent  State parent object or NULL
 * @param _initial State initial transition object or NULL
 */

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
#  ifdef CONFIG_SMF_INITIAL_TRANSITION
#    define SMF_CREATE_STATE(_entry, _run, _exit, _parent, _initial) \
       { \
         .entry   = (_entry), \
         .run     = (_run), \
         .exit    = (_exit), \
         .parent  = (_parent), \
         .initial = (_initial), \
       }
#  else
#    define SMF_CREATE_STATE(_entry, _run, _exit, _parent, _initial) \
       { \
         .entry   = (_entry), \
         .run     = (_run), \
         .exit    = (_exit), \
         .parent  = (_parent), \
       }
#  endif
#else
#  define SMF_CREATE_STATE(_entry, _run, _exit, _parent, _initial) \
     { \
       .entry = (_entry), \
       .run   = (_run), \
       .exit  = (_exit), \
     }
#endif

/**
 * @brief Macro to cast user defined object to state machine
 *        context.
 *
 * @param o A pointer to the user defined object
 */
#define SMF_CTX(o) ((struct smf_ctx *)o)

/**
 * @brief enum for the return value of a state_execution function
 */
enum smf_state_result {
	SMF_EVENT_HANDLED,
	SMF_EVENT_PROPAGATE,
};

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/**
 * @brief Function pointer that implements a entry and exit actions
 *        of a state
 *
 * @param obj pointer user defined object
 */
typedef void (*state_method)(void *obj);

/**
 * @brief Function pointer that implements a the run action of a state
 *
 * @param obj pointer user defined object
 * @return If the event should be propagated to parent states or not
 *         (Ignored when CONFIG_SMF_ANCESTOR_SUPPORT not defined)
 */
typedef enum smf_state_result (*state_execution)(void *obj);

/** General state that can be used in multiple state machines. */
struct smf_state {
	/** Optional method that will be run when this state is entered */
	const state_method entry;

	/**
	 * Optional method that will be run repeatedly during state machine
	 * loop.
	 */
	const state_execution run;

	/** Optional method that will be run when this state exists */
	const state_method exit;
#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	/**
	 * Optional parent state that contains common entry/run/exit
	 *	implementation among various child states.
	 *	entry: Parent function executes BEFORE child function.
	 *	run:   Parent function executes AFTER child function.
	 *	exit:  Parent function executes AFTER child function.
	 *
	 *	Note: When transitioning between two child states with a shared
	 *      parent,	that parent's exit and entry functions do not execute.
	 */
	const struct smf_state *parent;

#ifdef CONFIG_SMF_INITIAL_TRANSITION
	/**
	 * Optional initial transition state. NULL for leaf states.
	 */
	const struct smf_state *initial;
#endif /* CONFIG_SMF_INITIAL_TRANSITION */
#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */
};

/** Defines the current context of the state machine. */
struct smf_ctx {
	/** Current state the state machine is executing. */
	const struct smf_state *current;
	/** Previous state the state machine executed */
	const struct smf_state *previous;

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	/** Currently executing state (which may be a parent) */
	const struct smf_state *executing;
#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */
	/**
	 * This value is set by the set_terminate function and
	 * should terminate the state machine when its set to a
	 * value other than zero when it's returned by the
	 * run_state function.
	 */
	int32_t terminate_val;
	/**
	 * The state machine casts this to a "struct internal_ctx" and it's
	 * used to track state machine context
	 */
	uint32_t internal;
};

/**
 * @brief Initializes the state machine and sets its initial state.
 *
 * @param ctx        State machine context
 * @param init_state Initial state the state machine starts in.
 */
void smf_set_initial(struct smf_ctx *ctx, const struct smf_state *init_state);

/**
 * @brief Changes a state machines state. This handles exiting the previous
 *        state and entering the target state. For HSMs the entry and exit
 *        actions of the Least Common Ancestor will not be run.
 *
 * @param ctx       State machine context
 * @param new_state State to transition to
 */
void smf_set_state(struct smf_ctx *ctx, const struct smf_state *new_state);

/**
 * @brief Terminate a state machine
 *
 * @param ctx  State machine context
 * @param val  Non-Zero termination value that's returned by the smf_run_state
 *             function.
 */
void smf_set_terminate(struct smf_ctx *ctx, int32_t val);

/**
 * @brief Runs one iteration of a state machine (including any parent states)
 *
 * @param ctx  State machine context
 * @return	   A non-zero value should terminate the state machine. This
 *			   non-zero value could represent a terminal state being reached
 *			   or the detection of an error that should result in the
 *			   termination of the state machine.
 */
int32_t smf_run_state(struct smf_ctx *ctx);

/**
 * @brief Get the current leaf state.
 *
 * @note This may be a PARENT state if the HSM is malformed
 *		 (i.e. the initial transitions are not set up correctly).
 *
 * @param ctx State machine context
 * @return    The current leaf state.
 */
static inline const struct smf_state *smf_get_current_leaf_state(const struct smf_ctx *const ctx)
{
	return ctx->current;
}

/**
 * @brief Get the state that is currently executing. This may be a parent state.
 *
 * @param ctx State machine context
 * @return    The state that is currently executing.
 */
static inline const struct smf_state *
smf_get_current_executing_state(const struct smf_ctx *const ctx)
{
#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	return ctx->executing;
#else
	return ctx->current;
#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */
}

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_INCLUDE_SMF_SMF_H */
