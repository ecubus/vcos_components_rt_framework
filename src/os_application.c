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
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
#include "os_ipicall_internal.h"
#endif
#include "os_application.h"
#include "os_task.h"
#include "os_core.h"
#include "os_isr.h"

#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
#include "os_counter.h"
#endif

#define OS_START_SEC_CODE
#include "os_memmap.h"

FUNC(void, OS_CODE) os_app_init(const os_app_cfg_t *const os_app_cfg_table[], OsObjIndexType size)
{
	const os_app_cfg_t *app_cfg;
	OsObjIndexType index;
	CoreIdType core_id = os_get_core_id();

	if ((os_app_cfg_table == NULL) || (size == (OsObjIndexType)0)) {
		return;
	}

	for (index = 0; index < size; index++) {
		app_cfg = os_app_cfg_table[index];
		if ((app_cfg == NULL) || (app_cfg->core_id != core_id)) {
			continue;
		}
		if ((app_cfg->task_refs != NULL_PTR) && (app_cfg->task_count != NULL_PTR)) {
			os_task_init(app_cfg->task_refs, *(app_cfg->task_count));
		}
		if ((app_cfg->isr_refs != NULL_PTR) && (app_cfg->isr_count != NULL_PTR)) {
			os_isr_init(app_cfg->isr_refs, *(app_cfg->isr_count));
		}

#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
		if ((app_cfg->counter_refs != NULL_PTR) && (app_cfg->counter_count != NULL_PTR)) {
			os_counter_init(app_cfg->counter_refs, *(app_cfg->counter_count));
			os_counter_start(app_cfg->counter_refs, *(app_cfg->counter_count));
		}
#endif
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
		if ((app_cfg->scht_refs != NULL_PTR) && (app_cfg->scht_count != NULL_PTR)) {
			os_schedule_table_init(app_cfg->scht_refs, *(app_cfg->scht_count));
		}
#endif
		app_cfg->instance->state = APPLICATION_ACCESSIBLE;
	}
}

FUNC(ApplicationType, OS_CODE) os_get_app_id(void)
{
	const os_ctx_cfg_t *current_ctx_ref;

	current_ctx_ref = os_context_get_contexthdl();

	return current_ctx_ref->app_owner_cfg->app_id;
}

FUNC(ApplicationType, OS_CODE) os_get_current_app_id(void)
{
	const os_ctx_cfg_t *current_ctx_ref;

	current_ctx_ref = os_context_get_contexthdl();

	return ((os_ctx_inst_t *)(current_ctx_ref->instance))->current_app_cfg->app_id;
}

static FUNC(ApplicationStateType, OS_CODE) os_app_get_state(const os_app_cfg_t *app_ref)
{
	return app_ref->instance->state;
}

FUNC(Os_StatusType, OS_CODE) os_allow_access(void)
{
	const os_ctx_cfg_t *current_ctx_ref;
	const os_app_cfg_t *current_app_ref;

	current_ctx_ref = os_context_get_contexthdl();
	current_app_ref = ((os_ctx_inst_t *)(current_ctx_ref->instance))->current_app_cfg;

	if (!os_context_check_curcontext_id(current_ctx_ref, OS_CONTEXT_ALLOWACCESS)) {
        return OS_STATUS_CALLEVEL;
    }

    if (os_context_check_irq_disabled(current_ctx_ref)) {
        return OS_STATUS_DISABLEDINT;
    }

    if (os_app_get_state(current_app_ref) != APPLICATION_RESTARTING) {
        return OS_STATUS_STATE_NO_RESTART;
    }

    current_app_ref->instance->state = APPLICATION_ACCESSIBLE;
    return OS_STATUS_OK;
}

FUNC(Os_StatusType, OS_CODE) os_get_app_state(ApplicationType app_id, ApplicationStateRefType state)
{
	const os_ctx_cfg_t *const current_ctx = os_context_get_contexthdl();
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
	if (UNLIKELY(app_id >= os_app_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
#endif
	if (UNLIKELY(state == NULL_PTR)) {
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (UNLIKELY(os_context_check_curcontext_id(current_ctx, OS_CONTEXT_GETAPPLICATIONSTATE) == (boolean)FALSE)) {
		return OS_STATUS_CALLEVEL;
	}
	if (UNLIKELY(os_context_check_irq_disabled(current_ctx) == (boolean)TRUE)) {
		return OS_STATUS_DISABLEDINT;
	}
	const os_app_cfg_t *app_cfg = os_app_cfg_ref_table[app_id];
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
	if (UNLIKELY(app_cfg == NULL)) {
		return OS_STATUS_ID_1;
	}
#endif
	CoreIdType core_id = app_cfg->core_id;

	if (core_id == os_get_core_id()) {
		*state = app_cfg->instance->state;
		return OS_STATUS_OK;
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_get_app_state((uint32)core_id, app_id, state);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
}

FUNC(ApplicationType, OS_CODE) os_check_object_ownership(ObjectTypeType object_type, Os_ObjectIdType object_id)
{
	ApplicationType rv = (ApplicationType)INVALID_OSAPPLICATION;

	switch (object_type) {
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
		case OBJECT_COUNTER:
			if ((CounterType)object_id >= (CounterType)os_counter_cfg_ref_table_size) {
				break;
			} else {
				rv = GET_APP_CFG_BY_COUNTER_ID((CounterType)object_id)->app_id;
			}
			break;
#endif
		case OBJECT_ISR:
			if (object_id >= os_isr_cfg_ref_table_size) {
				break;
			} else {
				rv = GET_APP_CFG_BY_ISRREF_ID(object_id)->app_id;
			}
			break;
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
		case OBJECT_SCHEDULETABLE:
			if ((ScheduleTableType)object_id >= os_schedtable_cfg_ref_table_size) {
				break;
			} else {
				rv = GET_APP_CFG_BY_SCHEDULETABLE_ID((ScheduleTableType)object_id)->app_id;
			}
			break;
#endif /* CONFIG_RT_FRAMEWORK_SCHTBL */
		case OBJECT_TASK:
			if ((TaskType)object_id >= os_task_cfg_ref_table_size) {
				break;
			} else {
				rv = GET_APP_CFG_BY_TASK_ID((TaskType)object_id)->app_id;
			}
			break;
		default:
			break;
	}
	return rv;
}

#define OS_STOP_SEC_CODE
#include "os_memmap.h"
