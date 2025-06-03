/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 * Licensed under the Apache License, Version 2.0(the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OS_INTERRUPT_TYPES_H
#define OS_INTERRUPT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include "Os_Types.h"
#include "Std_Types.h"

typedef struct {
	os_int_api_cnt_t in_disableall; /*!< Inside a DisableAllInterrupts() API sequence, max value is 1. */
	os_int_api_cnt_t suspendall_cnt; /*! Nesting counter for SuspendAllInterrupts()/ResumeAllInterrupts(). */
	os_int_api_cnt_t suspendos_cnt; /*! Nesting counter for SuspendOSInterrupts()/ResumeOSInterrupts(). */
} os_int_lock_state_t;

typedef struct {
	/*! Interrupt state to be used for global lock. */
	os_int_global_state_t global;

	/*! Interrupt state to be used for level lock. */
	os_int_level_t level;
} os_int_state_t;

struct os_int_api_state {
	/*! Holds the previous interrupt state for DisableAllInterrupts() and SuspendAllInterrupts(). */
	os_int_state_t suspendall_prev_state;

	/*! Holds the previous interrupt level for SuspendOSInterrupts(). */
	os_int_state_t suspendos_prev_state;

	/*! Holds the nesting counters and flag for interrupt locking state. */
	os_int_lock_state_t lock_state;

	/*! \brief    Snapshot of nesting counter for SuspendAllInterrupts().
    *  \details  This is needed to detect nesting not in pairs. */
	os_int_api_cnt_t suspendall_snapshot;

	/*! \brief    Snapshot of nesting counter for SuspendOSInterrupts().
    *  \details  This is needed to detect nesting not in pairs. */
	os_int_api_cnt_t suspendos_snapshot;
};

/*! Management information for the interrupt API. */
typedef struct os_int_api_state os_int_api_state_t;


#ifdef __cplusplus
}
#endif

#endif /* OS_INTERRUPT_TYPES_H */
