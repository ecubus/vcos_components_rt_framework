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
#include <nuttx/arch.h>
#include "rtfw_interrupt.h"
#include "rtfw_core_internal.h"
#include "rtfw_context_internal.h"
#include "rtfw_isr.h"
#include "isr_cfg.h"
#include "rtfw_error.h"

/* Local Function Implementation */

static inline void os_int_suspend_cat1(os_int_state_t *const cat1_state)
{
#if defined(CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL) && (CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL == 1)
	cat1_state->level = up_irq_level_save(CONFIG_RT_FRAMEWORK_MAX_CAT1_INT_LEVEL);
#else
	cat1_state->global = up_irq_save();
#endif
}

static inline void os_int_resume_cat1(const os_int_state_t *const cat1_state)
{
#if defined(CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL) && (CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL == 1)
	up_irq_level_restore(cat1_state->level);
#else
	up_irq_restore(cat1_state->global);
#endif
}

static inline void os_int_suspend_cat2(os_int_state_t *const cat2_state)
{
#if defined(CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL) && (CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL == 1)
	cat2_state->level = up_irq_level_save(CONFIG_RT_FRAMEWORK_MAX_CAT2_INT_LEVEL);
#else
	cat2_state->global = up_irq_save();
#endif
}

static inline void os_int_resume_cat2(const os_int_state_t *const cat2_state)
{
#if defined(CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL) && (CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL == 1)
	up_irq_level_restore(cat2_state->level);
#else
	up_irq_restore(cat2_state->global);
#endif
}

/***********************************************************************************************************************
 *  分界线
 **********************************************************************************************************************/
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) os_disable_all_interrupts(void)
{
	os_status_t status = OS_STATUS_OK;
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_critical_section(int_state) == TRUE)) {
		status = OS_STATUS_INT_API_ORDER;
	} else {
		os_int_suspend_cat1(&(int_state->suspend_all_prev_state));
		os_interrupt_disable_all_enter(int_state);
	}

	REPORT_SERVICE_ERROR(OSServiceId_DisableAllInterrupts, status, 0);
}

FUNC(void, OS_CODE) os_enable_all_interrupts(void)
{
	os_status_t status = OS_STATUS_OK;
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_disable_all(int_state) == FALSE)) {
		status = OS_STATUS_INT_API_ORDER;
	} else {
		os_interrupt_disable_all_exit(int_state);
		os_int_resume_cat1(&(int_state->suspend_all_prev_state));
	}
	REPORT_SERVICE_ERROR(OSServiceId_EnableAllInterrupts, status, 0);
}

FUNC(void, OS_CODE) os_suspend_all_interrupts(void)
{
	os_status_t status = OS_STATUS_OK;
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_disable_all(int_state) == TRUE)) {
		status = OS_STATUS_INT_API_ORDER;
	} else {
		if (LIKELY(int_state->lock_state.suspend_all_cnt == 0u)) {
			os_int_suspend_cat1(&int_state->suspend_all_prev_state);
			int_state->suspend_os_snapshot = int_state->lock_state.suspend_os_cnt;
		}
		int_state->lock_state.suspend_all_cnt++;
		if (int_state->lock_state.suspend_all_cnt == 0) {
			status = OS_STATUS_INT_API_OVERFLOW;
		}
	}

	REPORT_SERVICE_ERROR(OSServiceId_SuspendAllInterrupts, status, 0);
}

FUNC(void, OS_CODE) os_resume_all_interrupts(void)
{
	os_status_t status = OS_STATUS_OK;
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_suspend_all(int_state) == FALSE)) {
		status = OS_STATUS_INT_API_ORDER;
	} else {
		int_state->lock_state.suspend_all_cnt--;
		if (LIKELY(int_state->lock_state.suspend_all_cnt == 0u)) {
			if (LIKELY(int_state->suspend_os_snapshot ==
				int_state->lock_state.suspend_os_cnt)) {
				os_int_resume_cat1(&(int_state->suspend_all_prev_state));
			} else {
				status = OS_STATUS_INT_API_ORDER;
			}
		}
	}

	REPORT_SERVICE_ERROR(OSServiceId_ResumeAllInterrupts, status, 0);
}

FUNC(void, OS_CODE) os_suspend_os_interrupts(void)
{
	os_status_t status = OS_STATUS_OK;
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_disable_all(int_state) == TRUE)) {
		status = OS_STATUS_INT_API_ORDER;
	} else {
		if (LIKELY(int_state->lock_state.suspend_os_cnt == 0u)) {
			os_int_suspend_cat2(&(int_state->suspend_os_prev_state));
			int_state->suspend_all_snapshot = int_state->lock_state.suspend_all_cnt;
		}
		int_state->lock_state.suspend_os_cnt++;
		if (int_state->lock_state.suspend_os_cnt == 0) {
			status = OS_STATUS_INT_API_OVERFLOW;
		}
	}

	REPORT_SERVICE_ERROR(OSServiceId_SuspendOSInterrupts, status, 0);
}

FUNC(void, OS_CODE) os_resume_os_interrupts(void)
{
	os_status_t status = OS_STATUS_OK;
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_suspend_os(int_state) == FALSE)) {
		status = OS_STATUS_INT_API_ORDER;
	} else {
		int_state->lock_state.suspend_os_cnt--;
		if (LIKELY(int_state->lock_state.suspend_os_cnt == 0u)) {
			if (LIKELY(int_state->suspend_all_snapshot ==
				int_state->lock_state.suspend_all_cnt)) {
				os_int_resume_cat2(&(int_state->suspend_os_prev_state));
			} else {
				status = OS_STATUS_INT_API_ORDER;
			}
		}
	}

	REPORT_SERVICE_ERROR(OSServiceId_ResumeOSInterrupts, status, 0);
}

FUNC(void, OS_CODE) os_interrupt_init(os_int_api_state_t *const int_state)
{
	if (int_state != NULL_PTR) {
		memset(int_state, 0, sizeof(*int_state));
	}
}

FUNC(boolean, OS_CODE) os_interrupt_is_disabled(void)
{
	return os_interrupt_is_in_critical_section(os_core_get_interrupt_api_stateref());
}

FUNC(void, OS_CODE) os_int_suspend_kernel(os_int_state_t *const int_state)
{
	irqlevel_t irq_state = up_kernel_int_save();
#if defined(CONFIG_RT_FRAMEWORK_INT_LEVEL) && (CONFIG_RT_FRAMEWORK_INT_LEVEL == 1)
	int_state->level = irq_state;
#else
	int_state->global = irq_state;
#endif
}

FUNC(void, OS_CODE) os_int_resume_kernel(const os_int_state_t *const int_state)
{
	irqlevel_t irq_state;
#if defined(CONFIG_RT_FRAMEWORK_INT_LEVEL) && (CONFIG_RT_FRAMEWORK_INT_LEVEL == 1)
	irq_state = int_state->level;
#else
	irq_state = int_state->global;
#endif
	up_kernel_int_restore(irq_state);
}

FUNC(void, OS_CODE) os_int_disable_global(void)
{
	up_irq_disable();
}

FUNC(void, OS_CODE) os_int_enable_global(void)
{
	up_irq_enable();
}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
static FUNC(os_status_t, OS_CODE) os_interrupt_access_check(const os_isr_cfg_t *IsrCfgRef)
{
	const os_ctx_cfg_t *const current_ctx = os_context_get_contexthdl();

	if (OS_STATUS_OK !=
	    os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_ctx), IsrCfgRef->context.access_app_mask)) {
		return OS_STATUS_ACCESS_RIGHT;
	} else {
		return OS_STATUS_OK;
	}
}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

FUNC(os_status_t, OS_CODE) os_enable_interrupt_source(isr_id_t isr_id, boolean clear_pending)
{
	os_status_t status = OS_STATUS_NOT_OK;

	/* Only if OsStatus is EXTENDED */
	if (isr_id >= OS_ISR_CNT) {
		status = OS_STATUS_ID_1;
	}
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	else if (os_app_check_is_accessible(GET_APP_CFG_BY_ISR_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESS_RIGHT;
	}
#endif
	else {
		uint32 IrqNumber = OS_ISR_GET_ISRREF_BY_ID(isr_id)->arch_isr_cfg.vector_num;

		if (up_irq_is_enabled((int)IrqNumber)) {
			status = OS_STATUS_ORDER;
		}
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
		else if ((os_get_curcontext_id() & OS_CONTEXT_ISRENABLESOURCE) == 0U) {
			status = OS_STATUS_CALLEVEL;
		}
#endif
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		else if (os_interrupt_access_check(OS_ISR_GET_ISRREF_BY_ID(isr_id)) != OS_STATUS_OK) {
			status = OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
		else {
			if (clear_pending) {
				up_clear_irq((int)IrqNumber);
			}

			up_enable_irq((int)IrqNumber);
			status = OS_STATUS_OK;
		}
	}

	return status;
}

FUNC(os_status_t, OS_CODE) os_disable_interrupt_source(isr_id_t isr_id)
{
	os_status_t status = OS_STATUS_NOT_OK;

	if (isr_id >= OS_ISR_CNT) {
		status = OS_STATUS_ID_1;
	}
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	else if (os_app_check_is_accessible(GET_APP_CFG_BY_ISR_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESS_RIGHT;
	}
#endif
	else {
		uint32 IrqNumber = OS_ISR_GET_ISRREF_BY_ID(isr_id)->arch_isr_cfg.vector_num;

		if (!up_irq_is_enabled((int)IrqNumber)) {
			status = OS_STATUS_ORDER;
		}
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
		else if ((os_get_curcontext_id() & OS_CONTEXT_ISRDISABLESOURCE) == 0U) {
			status = OS_STATUS_CALLEVEL;
		}
#endif
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		else if (os_interrupt_access_check(OS_ISR_GET_ISRREF_BY_ID(isr_id)) != OS_STATUS_OK) {
			status = OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
		else {
			up_disable_irq((int)IrqNumber);
			status = OS_STATUS_OK;
		}
	}

	/* Call ErrorHook if error happens */
	return status;
}

FUNC(os_status_t, OS_CODE) os_clear_pending_interrupt(isr_id_t isr_id)
{
	os_status_t status = OS_STATUS_NOT_OK;

	if (isr_id >= OS_ISR_CNT) {
		status = OS_STATUS_ID_1;
	}
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
	else if ((os_get_curcontext_id() & OS_CONTEXT_ISRDISABLESOURCE) == 0U) {
		status = OS_STATUS_CALLEVEL;
	}
#endif
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	else if (os_app_check_is_accessible(GET_APP_CFG_BY_ISR_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESS_RIGHT;
	} else if (os_interrupt_access_check(OS_ISR_GET_ISRREF_BY_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
	else {
		uint32 IrqNumber = OS_ISR_GET_ISRREF_BY_ID(isr_id)->arch_isr_cfg.vector_num;

		up_clear_irq((int)IrqNumber);
		status = OS_STATUS_OK;
	}

	return status;
}

FUNC(void, OS_CODE) os_disable_level_km(void)
{
	up_irq_disable();
}

FUNC(void, OS_CODE) os_enable_level_km(void)
{
	up_irq_enable();
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
