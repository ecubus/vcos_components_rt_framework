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

#ifndef RT_FRAMEWORK_INC_OS_INTERRUPT_CBK_H
#define RT_FRAMEWORK_INC_OS_INTERRUPT_CBK_H
#include <nuttx/config.h>
#include "rtfw_interrupt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 判断是否在临界区内(既已经关了中断) TRUE:是  FALSE: 否 */
static inline boolean os_interrupt_is_in_critical_section(const os_int_api_state_t *const int_state)
{
	return (boolean)(((int_state->lock_state.in_disableall) |
			  int_state->lock_state.suspend_all_cnt |
			  int_state->lock_state.suspend_os_cnt) != 0u);
}

static inline boolean os_interrupt_is_in_disable_all(const os_int_api_state_t *const int_state)
{
	return (boolean)(int_state->lock_state.in_disableall == 1u);
}

static inline boolean os_interrupt_is_in_suspend_all(const os_int_api_state_t *const int_state)
{
	return (boolean)(int_state->lock_state.suspend_all_cnt != 0u);
}

static inline boolean os_interrupt_is_in_suspend_os(const os_int_api_state_t *const int_state)
{
	return (boolean)(int_state->lock_state.suspend_os_cnt != 0u);
}

static inline void os_interrupt_disable_all_enter(os_int_api_state_t *const int_state)
{
	int_state->lock_state.in_disableall = 1;
}

static inline void os_interrupt_disable_all_exit(os_int_api_state_t *const int_state)
{
	int_state->lock_state.in_disableall = 0;
}

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_INTERRUPT_CBK_H */
