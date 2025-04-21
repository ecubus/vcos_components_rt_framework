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

/* Including File */
#include <nuttx/config.h>
#include "os_schedtable.h"
#include "os_event.h"
#include "os_task.h"
#include "os_core.h"
#include "os_counter.h"
#include "schedtable_cfg.h"
#include "application_cfg.h"
#include "core_cfg.h"

/* Local Macro Definition */
#define SCHED_CHECK_ID(x)                                                                                              \
	((os_schedtable_cfg_ref_table_size != 0UL) && ((x) < os_schedtable_cfg_ref_table_size) &&                      \
	 (os_schedtable_cfg_ref_table[x]->instance->schedtable_id < os_schedtable_cfg_ref_table_size))
#ifndef MIN
#define MIN(_x, _y) (((_x) < (_y)) ? (_x) : (_y))
#endif

/* Local Enumeration Definition */

/* Local type & Structure Definition */

/* global Variable Definition */

/* Local Variable Definition */

/* Local Function Declaration */
#define OS_START_SEC_CODE
#include "os_memmap.h"
static FUNC(Os_StatusType, OS_CODE) os_start_scheduletable_rel_startup(ScheduleTableType sid, TickType offset);
static FUNC(Os_StatusType, OS_CODE) os_start_schedule_table_abs_startup(ScheduleTableType sid, TickType start);
static FUNC(Os_StatusType, OS_CODE) os_start_schedule_table_synchron_startup(ScheduleTableType schedtable_id);
#define OS_STOP_SEC_CODE
#include "os_memmap.h"

/* Local Function Implementation */
static inline os_schedtable_inst_t *os_get_sched_table_by_id(ScheduleTableType schedtable_id)
{
	return os_schedtable_cfg_ref_table[schedtable_id]->instance;
}

static inline const os_schedtable_cfg_t *os_get_sched_table_cfg_by_id(ScheduleTableType schedtable_id)
{
	return os_schedtable_cfg_ref_table[schedtable_id];
}

static inline TickType os_get_sched_table_initial_offset(const os_schedtable_inst_t *schtbl_ptr)
{
	const os_schedtable_cfg_t *cfg_ptr = os_get_sched_table_cfg_by_id(schtbl_ptr->schedtable_id);

	return EXPIRE_POINT_GET(cfg_ptr, 0)->offset;
}

static inline TickType os_get_sched_table_max_value(const os_schedtable_inst_t *schtbl_ptr)
{
	ScheduleTableType schedtable_id = schtbl_ptr->schedtable_id;
	return os_schedtable_cfg_ref_table[schedtable_id]->action_config.counter->max_allowed_value;
}

static inline boolean os_sched_table_is_next_sync_point(const os_schedtable_inst_t *stbl,
							const os_schedtable_cfg_t *cfg_ptr)
{
	/* The tail node will not adjust the virtual tail node */
	if (stbl->current_expiry_index == (cfg_ptr->expiry_point_cnt - 1U)) {
		return (boolean)FALSE;
	}
	/* Virtual tail node, check if the first node is an explicit synchronization point */
	else if (stbl->current_expiry_index == cfg_ptr->expiry_point_cnt) {
		if ((EXPIRE_POINT_GET(cfg_ptr, 0U)->adjust_expiry_point.max_shorten != 0U) ||
		    (EXPIRE_POINT_GET(cfg_ptr, 0U)->adjust_expiry_point.max_lengthen != 0U)) {
			return (boolean)TRUE;
		} else {
			return (boolean)FALSE;
		}
	}
	/* current_expiry_index range = [0, (cfg_ptr->expiry_point_cnt - 2)] */
	else {
		if ((EXPIRE_POINT_GET(cfg_ptr, (stbl->current_expiry_index + 1U))->adjust_expiry_point.max_shorten !=
		     0U) ||
		    (EXPIRE_POINT_GET(cfg_ptr, (stbl->current_expiry_index + 1U))->adjust_expiry_point.max_lengthen !=
		     0U)) {
			return (boolean)TRUE;
		} else {
			return (boolean)FALSE;
		}
	}
}

static inline TickType os_get_sched_table_max_length_of_next(const os_schedtable_inst_t *stbl,
							     const os_schedtable_cfg_t *cfg_ptr)
{
	/* Tail node will not adjust virtual tail node, direct error */
	if (stbl->current_expiry_index == (cfg_ptr->expiry_point_cnt - 1U)) {
		// ReportFatalError();
		return 0U;
	}
	/* Virtual tail node, check the value of the first node */
	else if (stbl->current_expiry_index == cfg_ptr->expiry_point_cnt) {
		return EXPIRE_POINT_GET(cfg_ptr, 0)->adjust_expiry_point.max_lengthen;
	}
	/* current_expiry_index range = [0, (cfg_ptr->expiry_point_cnt - 2)] */
	else {
		return EXPIRE_POINT_GET(cfg_ptr, (stbl->current_expiry_index + 1U))->adjust_expiry_point.max_lengthen;
	}
}

static inline TickType os_get_sched_table_max_shorten_of_next(const os_schedtable_inst_t *stbl,
							      const os_schedtable_cfg_t *cfg_ptr)
{
	/* Tail node will not adjust virtual tail node, direct error */
	if (stbl->current_expiry_index == (cfg_ptr->expiry_point_cnt - 1U)) {
		// ReportFatalError();
		return 0U;
	}
	/* Virtual tail node, check the value of the first node */
	else if (stbl->current_expiry_index == cfg_ptr->expiry_point_cnt) {
		return EXPIRE_POINT_GET(cfg_ptr, 0)->adjust_expiry_point.max_shorten;
	}
	/* current_expiry_index range = [0, (cfg_ptr->expiry_point_cnt - 2)] */
	else {
		return EXPIRE_POINT_GET(cfg_ptr, (stbl->current_expiry_index + 1U))->adjust_expiry_point.max_shorten;
	}
}

static inline void os_get_sched_table_sync_update_deviation(const os_schedtable_cfg_t *schedtable_cfg_ptr,
							    os_schedtable_inst_t *schedtable_ptr,
							    TickType sync_time_now)
{
	TickType sync_time_delta;
	TickType sync_time_delta_half_mod;
	TickType sync_time_calculated;
	TickType counter_time_delta;
	TickType counter_time_now;

	TickType duration_half = ((TickType)(schedtable_cfg_ptr->duration / 2U));
	TickType last_counter_timestamp = schedtable_ptr->sync.counter_time_stamp;
	TickType last_sync_timestamp = schedtable_ptr->sync.sync_time_stamp;

	counter_time_now = os_counter_get_user_value(schedtable_cfg_ptr->action_config.counter);

	/****************************** Calculate counter_time_delta ****************************************/
	counter_time_delta = counter_time_now - last_counter_timestamp;
	if (counter_time_now < last_counter_timestamp) { /* counter value overflow scenario */
		counter_time_delta += (schedtable_cfg_ptr->action_config.counter->max_allowed_value + 1U);
	}

	/*
	* Reason for subtracting only one cycle:
	* last_counter_timestamp is refreshed every schedule table cycle, the difference cannot exceed two Durations,
	* subtraction operation is more efficient than modulo operation
	*/
	if (counter_time_delta >= schedtable_cfg_ptr->duration) {
		counter_time_delta -= schedtable_cfg_ptr->duration;
	}

	/****************************** Calculate sync_time_calculated ***********************************/
	sync_time_calculated = last_sync_timestamp + counter_time_delta;
	if (sync_time_calculated >= schedtable_cfg_ptr->duration) {
		sync_time_calculated -= schedtable_cfg_ptr->duration;
	}

	/****************************** Calculate sync_time_delta ****************************************/
	if (sync_time_now >= sync_time_calculated) {
		sync_time_delta = sync_time_now - sync_time_calculated;
	} else {
		sync_time_delta = (schedtable_cfg_ptr->duration - sync_time_calculated) + sync_time_now;
	}

	/****************************** Calculate sync_time_delta_half_mod ********************************/
	/* Time difference can be positive or negative, sync_time_delta_half_mod range [-DurationHalf, DurationHalf-1] */
	if (sync_time_delta >= duration_half) {
		sync_time_delta_half_mod = sync_time_delta - schedtable_cfg_ptr->duration;
	} else {
		sync_time_delta_half_mod = sync_time_delta;
	}

	/****************************** Calculate deviation *******************************************/
	TickType result = schedtable_ptr->sync.deviation - sync_time_delta_half_mod;

	if ((schedtable_ptr->sync.deviation < duration_half)
		&& (sync_time_delta_half_mod >= duration_half)
		&& (result >= duration_half)) {

		/* 1. the deviation is positive
		*  2. the sync_time_delta_half_mod is negative
		*  3. result exceeds the duration_half limit ,then move the result to negative part
		*/
		result -= schedtable_cfg_ptr->duration;
	} else if ((schedtable_ptr->sync.deviation >= duration_half)
			&& (sync_time_delta_half_mod < duration_half)
			&& ((TickType)(-(TickDiffType)result) > duration_half)) {

		/* 1. the deviation is negative
		*  2. the duration_half is positive
		*  3. calculation result exceeds negative duration_half，then move the result to positive part
		*/
		result += schedtable_cfg_ptr->duration;
	} else {
		/* nothing to do */
	}

	schedtable_ptr->sync.deviation = result;

	/****************************** Record the current time for next calculation ****************************************/
	schedtable_ptr->sync.counter_time_stamp = counter_time_now;
	schedtable_ptr->sync.sync_time_stamp = sync_time_now;
}

static inline void os_get_sched_table_sync_update_status(const os_schedtable_cfg_t *schedtable_cfg_ptr)
{
	os_schedtable_inst_t *schedtable_ptr = schedtable_cfg_ptr->instance;

	if (schedtable_ptr->sync.sync_state == SCHTBL_SYNCSTATUS_PROVIDED) {
		if (schedtable_ptr->sync.deviation <= schedtable_cfg_ptr->precision) {
			schedtable_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
		} else if ((TickType)(-((TickDiffType)(schedtable_ptr->sync.deviation))) <= schedtable_cfg_ptr->precision ) {
			schedtable_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
		} else {
			schedtable_ptr->state = SCHEDULETABLE_RUNNING;
		}
	}
}

static inline void os_get_sched_table_reduce_deviation(os_schedtable_inst_t *stbl, const os_schedtable_cfg_t *cfg_ptr)
{
	TickType correction;
	TickType max_length_of_next = os_get_sched_table_max_length_of_next(stbl, cfg_ptr);
	TickType max_shorten_of_next = os_get_sched_table_max_shorten_of_next(stbl, cfg_ptr);

	if ((((TickDiffType)stbl->sync.deviation) > ((TickDiffType)0)) && (stbl->sync.deviation > max_length_of_next)) {
		/* 0 < max_length_of_next < deviation */
		correction = max_length_of_next;
	} else if ((((TickDiffType)stbl->sync.deviation) <= ((TickDiffType)0)) &&
		(stbl->sync.deviation < max_shorten_of_next)) {
		/* deviation < max_shorten_of_next < 0 */
		correction = max_shorten_of_next;
	} else {
		/* other conditions */
		correction = stbl->sync.deviation;
	}

	/* Single step deviation for subsequent execution */
	stbl->sync.one_step_adjust = correction;

	/* Record the remaining deviation to be eliminated */
	stbl->sync.deviation -= correction;
}

static inline void os_update_sched_table_sync_timestamp(os_schedtable_inst_t *stbl, const os_schedtable_cfg_t *cfg_ptr)
{
	TickType last_counter_timestamp = stbl->sync.counter_time_stamp;
	TickType last_sync_timestamp = stbl->sync.sync_time_stamp;

	TickType counter_time_now = os_counter_get_user_value(cfg_ptr->action_config.counter);

	/****************************** Calculate diff_counter ****************************************/
	TickType diff_counter = counter_time_now - last_counter_timestamp;

	if (counter_time_now < last_counter_timestamp) { /* Overflow scenario */
		diff_counter += (cfg_ptr->action_config.counter->max_allowed_value + 1U);
	}

	/*
	* Reason for subtracting only one cycle:
	* last_counter_timestamp is refreshed every schedule table cycle, the difference cannot exceed two Durations,
	* subtraction operation is more efficient than modulo operation
	*/
	if (diff_counter >= cfg_ptr->duration) {
		diff_counter -= cfg_ptr->duration;
	}

	/****************************** Update driving counter ****************************************/
	stbl->sync.counter_time_stamp = counter_time_now;
	stbl->sync.sync_time_stamp = last_sync_timestamp + diff_counter;
	if (stbl->sync.sync_time_stamp >= cfg_ptr->duration) {
		stbl->sync.sync_time_stamp -= cfg_ptr->duration;
	}
}

static inline void os_sched_table_sync_expiry_point(os_schedtable_inst_t *stbl, const os_schedtable_cfg_t *cfg_ptr)
{
	/* Update driving counter */
	os_update_sched_table_sync_timestamp(stbl, cfg_ptr);

	/* Eliminate deviation */
	if (stbl->sync.deviation != 0U) {
		os_get_sched_table_reduce_deviation(stbl, cfg_ptr);
	}

	/* Update schedule table status, external interface SyncScheduleTable may affect status transitions */
	os_get_sched_table_sync_update_status(cfg_ptr);
}

static inline void os_sched_table_sync_expiry_point_zero(os_schedtable_inst_t *stbl, const os_schedtable_cfg_t *cfg_ptr)
{
	/* Update driving counter */
	os_update_sched_table_sync_timestamp(stbl, cfg_ptr);

	/* Update schedule table status, external interface SyncScheduleTable may affect status transitions */
	os_get_sched_table_sync_update_status(cfg_ptr);
}

/* global Function Declaration */
#define OS_START_SEC_CODE
#include "os_memmap.h"

static FUNC(void, OS_CODE) os_update_sched_table_state(os_schedtable_inst_t *stbl)
{
	TickType delta;
	TickType inital_offset;
	TickType final_offset;
	os_schedtable_inst_t *next_schedtable_ptr;
	const os_schedtable_cfg_t *next_schedtable_cfg_ptr;
	const os_schedtable_cfg_t *cfg_ptr = os_get_sched_table_cfg_by_id(stbl->schedtable_id);

	/* For explicit synchronization schedule tables at each expiry point + virtual tail node, update the synchronization counter */
	if (cfg_ptr->sync_strategy == EXPLICIT) {
		if (os_sched_table_is_next_sync_point(stbl, cfg_ptr) == (boolean)TRUE) {
			/* Next expiry point is an explicit synchronization point */
			os_sched_table_sync_expiry_point(stbl, cfg_ptr);
		} else {
			/* Next expiry point is not an explicit synchronization point */
			os_sched_table_sync_expiry_point_zero(stbl, cfg_ptr);
		}
	}

	if ((stbl->current_expiry_index) == (cfg_ptr->expiry_point_cnt - 1U)) { /* At the last expiry point */
		final_offset = cfg_ptr->duration - EXPIRE_POINT_GET(cfg_ptr, cfg_ptr->expiry_point_cnt - 1UL)->offset;

		/* Tail node does not do explicit synchronization adjustment */
		os_counter_reload_action(&cfg_ptr->action_config, final_offset);
		stbl->current_expiry_index++;
		return;
	}

	if ((stbl->current_expiry_index) == cfg_ptr->expiry_point_cnt) { /* At final offset */
		if ((cfg_ptr->repeating == SCHTBL_REPEATING) || (stbl->next != NULL)) {
			if (stbl->next == NULL) {  /* Calculate the first expiry point of a periodic schedule table */

				inital_offset = os_get_sched_table_initial_offset(stbl);

				/* inital_offset and one_step_adjust satisfy the following conditions:
				* 1. delta is only positive
				* 2. one_step_adjust can be positive or negative
				* 3. |delta| > |one_step_adjust|
				*/
				if (stbl->sync.one_step_adjust != 0U) {
					os_counter_reload_action(&cfg_ptr->action_config,
								 inital_offset + stbl->sync.one_step_adjust);
					stbl->sync.one_step_adjust = 0U;
				} else {
					os_counter_reload_action(&cfg_ptr->action_config, inital_offset);
				}
			} else { /* Next schedule table */
				next_schedtable_ptr = stbl->next;

				inital_offset = os_get_sched_table_initial_offset(next_schedtable_ptr);
				stbl->state = SCHEDULETABLE_STOPPED;
				next_schedtable_ptr->state = SCHEDULETABLE_RUNNING;
				next_schedtable_cfg_ptr = os_get_sched_table_cfg_by_id(next_schedtable_ptr->schedtable_id);

				os_counter_add_rel_action(&next_schedtable_cfg_ptr->action_config, inital_offset);
			}
		} else {
			stbl->state = SCHEDULETABLE_STOPPED;
		}
		stbl->current_expiry_index = 0;
	} else { /* Calculate between multiple expiry points in a schedule table */
		delta = EXPIRE_POINT_GET(cfg_ptr, stbl->current_expiry_index + 1UL)->offset -
			EXPIRE_POINT_GET(cfg_ptr, stbl->current_expiry_index)->offset;

		/* delta and one_step_adjust satisfy the following conditions:
		* 1. delta is only positive
		* 2. one_step_adjust can be positive or negative
		* 3. |delta| > |one_step_adjust|
		*/
		if (stbl->sync.one_step_adjust != 0U) {
			os_counter_reload_action(&cfg_ptr->action_config, delta + stbl->sync.one_step_adjust);
			stbl->sync.one_step_adjust = 0U;
		} else {
			os_counter_reload_action(&cfg_ptr->action_config, delta);
		}

		stbl->current_expiry_index++;
	}

	return;
}

static FUNC(Os_StatusType, OS_CODE) os_sched_table_check_consistency(const os_schedtable_inst_t *schedtbl_ptr)
{
	const os_schedtable_cfg_t *cfg_ptr = os_get_sched_table_cfg_by_id(schedtbl_ptr->schedtable_id);

	if ((cfg_ptr->sync_strategy == IMPLICIT) &&
	    (cfg_ptr->duration != (os_get_sched_table_max_value(schedtbl_ptr) + 1UL))) {
		return OS_STATUS_NOT_OK;
	}

	if ((cfg_ptr->sync_strategy == EXPLICIT) &&
	    (cfg_ptr->duration > (os_get_sched_table_max_value(schedtbl_ptr) + 1UL))) {
		return OS_STATUS_NOT_OK;
	}

	if (cfg_ptr->expiry_point_cnt == 0UL) {
		return OS_STATUS_NOT_OK;
	}

	sint32 iter;
	TickType delta = 0;
	TickType max_value = os_get_sched_table_max_value(schedtbl_ptr);

	for (iter = 0; iter < (sint32)cfg_ptr->expiry_point_cnt; iter++) {
		delta = EXPIRE_POINT_GET(cfg_ptr, iter)->offset - delta;
		/* initial offset may be zero */
		if ((iter != 0) && (delta == 0U)) {
			return OS_STATUS_NOT_OK;
		}
		if (delta > max_value) {
			return OS_STATUS_NOT_OK;
		}
	}

	/* Final */
	delta = cfg_ptr->duration - EXPIRE_POINT_GET(cfg_ptr, iter - 1)->offset;
	if ((delta == 0UL) || (delta > max_value)) {
		return OS_STATUS_NOT_OK;
	}

	return OS_STATUS_OK;
}

FUNC(Os_StatusType, OS_CODE) os_sched_table_validity_paras(const os_schedtable_cfg_t *os_schedtable_cfg)
{
	const os_schedtable_inst_t *schedtable_ptr;
	Os_StatusType os_schedtbl_para_validity_result;

	if (os_schedtable_cfg == NULL) {
		return OS_STATUS_NOT_OK;
	}

	schedtable_ptr = os_schedtable_cfg->instance;

	os_schedtbl_para_validity_result = os_sched_table_check_consistency(schedtable_ptr);
	if (os_schedtbl_para_validity_result != OS_STATUS_OK) {
		return OS_STATUS_NOT_OK;
	}

	return OS_STATUS_OK;
}

FUNC(Os_StatusType, OS_CODE) os_start_schedule_table_rel(ScheduleTableType schedtable_id, TickType offset)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *schtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	TickType max_offset;
	os_int_state_t Int_state;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedtable_id, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		schtbl_ptr = os_get_sched_table_by_id(schedtable_id);
		cfg_ptr = os_get_sched_table_cfg_by_id(schtbl_ptr->schedtable_id);

		max_offset = os_get_sched_table_max_value(schtbl_ptr);
		if ((offset == (TickType)0) ||
		    ((offset + os_get_sched_table_initial_offset(schtbl_ptr)) > max_offset)) {
			rv = OS_STATUS_VALUE_IS_OUT_OF_BOUNDS_2;
			return rv;
		}

		os_int_suspend_kernel(&Int_state);
		if (schtbl_ptr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STATE_STOPPED;
		}

		os_counter_add_rel_action(&cfg_ptr->action_config,
					  offset + os_get_sched_table_initial_offset(schtbl_ptr));
		schtbl_ptr->state = SCHEDULETABLE_RUNNING;
		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_start_schedule_table_rel((uint32)coreId, schedtable_id, offset);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

FUNC(Os_StatusType, OS_CODE) os_start_schedule_table_abs(ScheduleTableType schedtable_id, TickType start)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *schedtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	os_int_state_t Int_state;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedtable_id, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		schedtbl_ptr = os_get_sched_table_by_id(schedtable_id);
		cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		if (start > os_get_sched_table_max_value(schedtbl_ptr)) {
			return OS_STATUS_VALUE_IS_OUT_OF_BOUNDS_1;
		}

		os_int_suspend_kernel(&Int_state);

		if (schedtbl_ptr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STATE_STOPPED;
		}

		os_counter_add_abs_action(&cfg_ptr->action_config,
					  os_counter_add(start, os_get_sched_table_initial_offset(schedtbl_ptr),
							 os_get_sched_table_max_value(schedtbl_ptr)));

		if (cfg_ptr->sync_strategy == IMPLICIT) {
			schedtbl_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
		} else {
			schedtbl_ptr->state = SCHEDULETABLE_RUNNING;
		}

		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_start_schedule_table_abs((uint32)coreId, schedtable_id, start);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

FUNC(Os_StatusType, OS_CODE) os_start_schedule_table_synchron(ScheduleTableType schedtable_id)
{
	os_schedtable_inst_t *schedtable_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	os_int_state_t Int_state;
	Os_StatusType rv = OS_STATUS_OK;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedtable_id, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);
		cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		if (cfg_ptr->sync_strategy != EXPLICIT) {
			return OS_STATUS_ID_DIFFERENT_SYNC;
		}

		os_int_suspend_kernel(&Int_state);

		if (schedtable_ptr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STATE_STOPPED;
		}

		schedtable_ptr->state = SCHEDULETABLE_WAITING;

		/*Initial variable */
		schedtable_ptr->sync.sync_state = SCHTBL_SYNCSTATUS_NOT_PROVIDED;
		schedtable_ptr->sync.deviation = 0;
		schedtable_ptr->sync.sync_time_stamp = 0;
		schedtable_ptr->sync.counter_time_stamp = 0;
		schedtable_ptr->sync.one_step_adjust = 0;

		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_start_schedule_table_synchron((uint32)coreId, schedtable_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

FUNC(Os_StatusType, OS_CODE) os_stop_schedule_table(ScheduleTableType schedtable_id)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *schtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	os_int_state_t Int_state;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedtable_id, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		schtbl_ptr = os_get_sched_table_by_id(schedtable_id);
		cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		os_int_suspend_kernel(&Int_state);

		if (schtbl_ptr->state == SCHEDULETABLE_STOPPED) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STOPPED_1;
		}

		if ((schtbl_ptr->state == SCHEDULETABLE_RUNNING) ||
		    (schtbl_ptr->state == SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)) {
			os_counter_delete_action(&cfg_ptr->action_config);
		}
		schtbl_ptr->state = SCHEDULETABLE_STOPPED;

		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_stop_schedule_table((uint32)coreId, schedtable_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

FUNC(Os_StatusType, OS_CODE)
os_next_schedule_table(ScheduleTableType schedule_table_id_from, ScheduleTableType schedule_table_id_to)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *sFromPtr;
	os_schedtable_inst_t *sToPtr;
	os_int_state_t Int_state;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedule_table_id_from) == (boolean)FALSE ||
	    SCHED_CHECK_ID(schedule_table_id_to) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedule_table_id_from, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		sFromPtr = os_get_sched_table_by_id(schedule_table_id_from);
		sToPtr = os_get_sched_table_by_id(schedule_table_id_to);

		os_int_suspend_kernel(&Int_state);

		if ((sFromPtr->state == SCHEDULETABLE_STOPPED) || (sFromPtr->state == SCHEDULETABLE_NEXT)) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STOPPED_1;
		}

		if (sToPtr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STATE_STOPPED;
		}

		if (sFromPtr->next != NULL) {
			sFromPtr->next->state = SCHEDULETABLE_STOPPED;
		}

		sFromPtr->next = sToPtr;
		sToPtr->state = SCHEDULETABLE_NEXT;
		sToPtr->current_expiry_index = 0;

		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_next_schedule_table((uint32)coreId, schedule_table_id_from, schedule_table_id_to);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

FUNC(Os_StatusType, OS_CODE)
os_next_schedule_table_from_stop(ScheduleTableType schedule_table_id_from, ScheduleTableType schedule_table_id_to)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *sFromPtr;
	os_schedtable_inst_t *sToPtr;
	os_int_state_t Int_state;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedule_table_id_from) == (boolean)FALSE ||
	    SCHED_CHECK_ID(schedule_table_id_to) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedule_table_id_from, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		sFromPtr = os_get_sched_table_by_id(schedule_table_id_from);
		sToPtr = os_get_sched_table_by_id(schedule_table_id_to);

		os_int_suspend_kernel(&Int_state);

		if (sFromPtr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STOPPED_1;
		}

		if (sToPtr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STATE_STOPPED;
		}

		if (sFromPtr->next != NULL) {
			sFromPtr->next->state = SCHEDULETABLE_STOPPED;
		}

		sFromPtr->next = sToPtr;
		sToPtr->state = SCHEDULETABLE_NEXT;
		sToPtr->current_expiry_index = 0;

		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_next_schedule_table_from_stop((uint32)coreId, schedule_table_id_from,
								schedule_table_id_to);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

FUNC(Os_StatusType, OS_CODE) os_sync_schedule_table(ScheduleTableType schedtable_id, TickType sync_time_now)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *schedtable_ptr;
	const os_schedtable_cfg_t *schedtable_cfg_ptr;
	TickType startTimeToWait = 0;
	os_int_state_t Int_state;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedtable_id, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);
		schedtable_cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		os_int_suspend_kernel(&Int_state);
		if ((schedtable_ptr->state == SCHEDULETABLE_STOPPED) || (schedtable_ptr->state == SCHEDULETABLE_NEXT)) {
			os_int_resume_global(&Int_state);
			return OS_STATUS_STATE_STOPPED;
		}

		switch (schedtable_ptr->state) {
			case SCHEDULETABLE_WAITING:
				schedtable_ptr->sync.deviation = 0;
				schedtable_ptr->sync.sync_time_stamp = sync_time_now;
				schedtable_ptr->sync.counter_time_stamp =
					os_counter_get_user_value(schedtable_cfg_ptr->action_config.counter);
				schedtable_ptr->sync.sync_state = SCHTBL_SYNCSTATUS_PROVIDED;
				schedtable_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
				schedtable_ptr->sync.one_step_adjust = 0;

				startTimeToWait =
					(schedtable_cfg_ptr->duration - sync_time_now) % schedtable_cfg_ptr->duration;

				os_counter_add_rel_action(&schedtable_cfg_ptr->action_config,
							  startTimeToWait +
								  os_get_sched_table_initial_offset(schedtable_ptr));
				break;

			case SCHEDULETABLE_RUNNING:
			case SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS:
				if (schedtable_ptr->sync.sync_state == SCHTBL_SYNCSTATUS_NOT_PROVIDED) {
					schedtable_ptr->sync.sync_state = SCHTBL_SYNCSTATUS_PROVIDED;
				}

				/* Update deviation */
				os_get_sched_table_sync_update_deviation(schedtable_cfg_ptr, schedtable_ptr,
									 sync_time_now);
				/* Update status */
				os_get_sched_table_sync_update_status(schedtable_cfg_ptr);

				break;

			default:
				/* do nothing */
				break;
		}

		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_sync_schedule_table((uint32)coreId, schedtable_id, sync_time_now);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

FUNC(Os_StatusType, OS_CODE)
os_get_schedule_table_status(ScheduleTableType schedtable_id, ScheduleTableStatusRefType ScheduleStatus)
{
	Os_StatusType rv = OS_STATUS_OK;
	const os_schedtable_inst_t *schedtable_ptr;
	os_int_state_t Int_state;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedtable_id, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);

		os_int_suspend_kernel(&Int_state);

		switch (schedtable_ptr->state) {
			case SCHEDULETABLE_STOPPED:
			case SCHEDULETABLE_NEXT:
			case SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS:
			case SCHEDULETABLE_WAITING:
			case SCHEDULETABLE_RUNNING:
				*ScheduleStatus = schedtable_ptr->state;
				break;
			default:
				/* do nothing */
				break;
		}

		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_get_schedule_table_status((uint32)coreId, schedtable_id, ScheduleStatus);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

FUNC(Os_StatusType, OS_CODE) os_set_schedule_table_async(ScheduleTableType schedtable_id)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *schedtable_ptr;
	os_int_state_t Int_state;
	CoreIdType coreId = OS_CORE_CNT;

	if (SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	rv = get_sched_table_core_id(schedtable_id, &coreId);
	if ((OS_STATUS_OK != rv) || (((OsObjIndexType)coreId) >= os_core_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (coreId == os_get_core_id()) {
		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);

		os_int_suspend_kernel(&Int_state);

		schedtable_ptr->state = SCHEDULETABLE_RUNNING;

		os_int_resume_global(&Int_state);
	} else {
#if (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_set_schedule_table_async((uint32)coreId, schedtable_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return rv;
}

static FUNC(Os_StatusType, OS_CODE) os_start_scheduletable_rel_startup(ScheduleTableType sid, TickType offset)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *schtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	TickType max_offset;
	os_int_state_t Int_state;

	if (SCHED_CHECK_ID(sid) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	schtbl_ptr = os_get_sched_table_by_id(sid);
	cfg_ptr = os_get_sched_table_cfg_by_id(sid);

	/* EXPLICIT or IMPLICIT */
	if (cfg_ptr->sync_strategy == IMPLICIT) {
		return OS_STATUS_ID_1;
	}

	max_offset = os_get_sched_table_max_value(schtbl_ptr);
	if ((offset == 0UL) || ((offset + os_get_sched_table_initial_offset(schtbl_ptr)) > max_offset)) {
		return OS_STATUS_VALUE_IS_OUT_OF_BOUNDS_2;
	}

	os_int_suspend_kernel(&Int_state);

	if (schtbl_ptr->state != SCHEDULETABLE_STOPPED) {
		os_int_resume_global(&Int_state);
		return OS_STATUS_STATE_STOPPED;
	}

	/* calculate the expire value.. */
	os_counter_add_rel_action(&cfg_ptr->action_config, offset + os_get_sched_table_initial_offset(schtbl_ptr));
	schtbl_ptr->state = SCHEDULETABLE_RUNNING;

	os_int_resume_global(&Int_state);

	return rv;
}

static FUNC(Os_StatusType, OS_CODE) os_start_schedule_table_abs_startup(ScheduleTableType sid, TickType start)
{
	Os_StatusType rv = OS_STATUS_OK;
	os_schedtable_inst_t *schedtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	os_int_state_t Int_state;

	if (SCHED_CHECK_ID(sid) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	schedtbl_ptr = os_get_sched_table_by_id(sid);
	cfg_ptr = os_get_sched_table_cfg_by_id(sid);

	if (start > os_get_sched_table_max_value(schedtbl_ptr)) {
		return OS_STATUS_VALUE_IS_TOO_HIGH_1;
	}

	os_int_suspend_kernel(&Int_state);

	if (schedtbl_ptr->state != SCHEDULETABLE_STOPPED) {
		os_int_resume_global(&Int_state);
		return OS_STATUS_STATE_STOPPED;
	}

	os_counter_add_abs_action(&cfg_ptr->action_config,
				  os_counter_add(start, os_get_sched_table_initial_offset(schedtbl_ptr),
						 os_get_sched_table_max_value(schedtbl_ptr)));

	if (cfg_ptr->sync_strategy == IMPLICIT) {
		schedtbl_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
	} else {
		schedtbl_ptr->state = SCHEDULETABLE_RUNNING;
	}

	os_int_resume_global(&Int_state);

	return rv;
}

static FUNC(Os_StatusType, OS_CODE) os_start_schedule_table_synchron_startup(ScheduleTableType schedtable_id)
{
	os_schedtable_inst_t *schedtable_ptr;
	os_int_state_t Int_state;

	Os_StatusType rv = OS_STATUS_OK;

	if (os_interrupt_is_disabled() == (boolean)TRUE) {
		return OS_STATUS_DISABLEDINT;
	}

	schedtable_ptr = os_get_sched_table_by_id(schedtable_id);

	os_int_suspend_kernel(&Int_state);

	if (schedtable_ptr->state != SCHEDULETABLE_STOPPED) {
		os_int_resume_global(&Int_state);
		return OS_STATUS_STATE_STOPPED;
	}

	schedtable_ptr->state = SCHEDULETABLE_WAITING;

	os_int_resume_global(&Int_state);

	return rv;
}

FUNC(void, OS_CODE)
os_schedule_table_init(const os_schedtable_cfg_t *const os_schedtable_cfg_table[], OsObjIndexType size)
{
	OsObjIndexType index;
	const os_schedtable_cfg_t *schTblCfg;
	os_counter_inst_t *cPtr;
	os_schedtable_inst_t *schtbl_ptr;
	Os_StatusType Os_SchedTableInit_result;
	const os_app_cfg_t *appCfg = NULL;
	CoreIdType core_id = os_get_core_id();

	if ((os_schedtable_cfg_table == NULL) || (size == 0U)) {
		return;
	}

	/* Initialize all schedule table IDs (maintain global consistency) */
	for (index = 0; index < size; index++) {
		schTblCfg = (const os_schedtable_cfg_t *)os_schedtable_cfg_table[index];
		schtbl_ptr = os_schedtable_cfg_table[index]->instance;
		schtbl_ptr->schedtable_id = schTblCfg->schedtable_id;
	}

	/* Connect schedule tables to counters - only process tables for current core */
	for (ScheduleTableType i = 0; i < size; i++) {
		schTblCfg = os_schedtable_cfg_table[i];
		appCfg = os_app_cfg_ref_table[schTblCfg->app_owner_id];

		/* Skip schedule tables not belonging to current core */
		if (appCfg->core_id != core_id) {
			continue;
		}

		Os_SchedTableInit_result = os_sched_table_validity_paras(os_schedtable_cfg_table[i]);
		if (Os_SchedTableInit_result != OS_STATUS_OK) {
			os_schedtable_cfg_table[i]->instance->schedtable_id = (uint16)OS_SCHED_TABLE_CNT;
			continue;
		}

		schtbl_ptr = os_schedtable_cfg_table[i]->instance;
		cPtr = schTblCfg->action_config.counter->obj_desc;
		schtbl_ptr->list_schedtable_node = cPtr->schedtable_list_first;
		cPtr->schedtable_list_first = schtbl_ptr;
	}

	/* Initialize auto-start schedule tables - only process tables for current core */
	for (ScheduleTableType j = 0; j < size; j++) {
		schTblCfg = os_schedtable_cfg_table[j];
		appCfg = os_app_cfg_ref_table[schTblCfg->app_owner_id];

		/* Skip schedule tables not belonging to current core */
		if (appCfg->core_id != core_id) {
			continue;
		}

		const os_schedtable_autostart_t *autoPtr = &(schTblCfg->autostart);

		if (autoPtr->app_mode == OSAUTOSTARTMODE) {
			if (autoPtr->type == SCHTBL_AUTOSTART_ABSOLUTE) {
				(void)os_start_schedule_table_abs_startup(schTblCfg->schedtable_id, autoPtr->offset);
			} else if (autoPtr->type == SCHTBL_AUTOSTART_RELATIVE) {
				(void)os_start_scheduletable_rel_startup(schTblCfg->schedtable_id, autoPtr->offset);
			} else if (autoPtr->type == SCHTBL_AUTOSTART_SYNCHRONE) {
				(void)os_start_schedule_table_synchron_startup(schTblCfg->schedtable_id);
			} else {
				/* do nothing */
			}
		}
	}
}

FUNC(void, OS_CODE) schedule_table_action_callback(struct hrtimer_s *hrtimer)
{
	const os_action_cfg_t *action = (const os_action_cfg_t *)(hrtimer->arg);
	const os_schedtable_cfg_t *cfg_ptr = OS_GET_SCHEDTABLE_CFG_BY_ACTION_ADDR(action);
	os_schedtable_inst_t *schedTablePtr = cfg_ptr->instance;

	if ((schedTablePtr->state == SCHEDULETABLE_RUNNING) ||
	    (schedTablePtr->state == SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)) {
		ScheduleTableType sid = schedTablePtr->schedtable_id;

		cfg_ptr = os_get_sched_table_cfg_by_id(sid);
		if (schedTablePtr->current_expiry_index < cfg_ptr->expiry_point_cnt) {
			const os_schedtable_expiry_point_t *exp_point;
			uint32 i;

			exp_point = EXPIRE_POINT_GET(cfg_ptr, schedTablePtr->current_expiry_index);

			for (i = 0; i < (uint32_t)exp_point->task_list_cnt; i++) {
				(void)os_activate_task_asyn(exp_point->task_list[i]);
			}

			for (i = 0; i < (uint32_t)exp_point->event_list_cnt; i++) {
				(void)os_set_event_asyn(exp_point->event_list[i].task, exp_point->event_list[i].mask);
			}
		}

		os_update_sched_table_state(schedTablePtr);
	}
}

FUNC(void, OS_CODE) os_schedule_table_kill(const os_schedtable_cfg_t *const schedtable_ref)
{
	os_stop_schedule_table(schedtable_ref->schedtable_id);
}

FUNC(Os_StatusType, OS_CODE) get_sched_table_core_id(ScheduleTableType schedtable_id, CoreIdType *core_id_ref)
{
	ApplicationIDType ApplID;
	const os_app_cfg_t *pAppCfg;
	const os_schedtable_cfg_t *pSchedTableCfg;

	if (core_id_ref == NULL) {
		return OS_STATUS_PARAM_POINTER_2;
	}

	if (schedtable_id >= os_schedtable_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	pSchedTableCfg = os_schedtable_cfg_ref_table[schedtable_id];

	if (pSchedTableCfg == NULL) {
		return OS_STATUS_ID_1;
	}

	ApplID = pSchedTableCfg->app_owner_id;
	if (ApplID >= os_app_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	pAppCfg = os_app_cfg_ref_table[ApplID];

	if (pAppCfg == NULL) {
		return OS_STATUS_ID_1;
	}

	*core_id_ref = pAppCfg->core_id;

	return OS_STATUS_OK;
}
#define OS_STOP_SEC_CODE
#include "os_memmap.h"
