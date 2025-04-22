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
#include "os_interrupt.h"
#include "os_core_internal.h"
#include "os_context_internal.h"
#include "os_isr.h"
#include "isr_cfg.h"

/* Local Function Implementation */

static inline void os_int_suspend_cat1(os_int_state_t *const cat1_state)
{
	cat1_state->global = up_irq_save();
}

static inline void os_int_resume_cat1(const os_int_state_t *const cat1_state)
{
	up_irq_restore(cat1_state->global);
}

static inline void os_int_suspend_cat2(os_int_state_t *const cat2_state)
{
	cat2_state->global = up_irq_save();
}

static inline void os_int_resume_cat2(const os_int_state_t *const cat2_state)
{
	up_irq_restore(cat2_state->global);
}

/***********************************************************************************************************************
 *  分界线
 **********************************************************************************************************************/
#define OS_START_SEC_CODE
#include "os_memmap.h"

FUNC(void, OS_CODE) os_disable_all_interrupts(void)
{
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_critical_section(int_state) == TRUE)) {
		return;
	}
	os_int_suspend_cat1(&(int_state->suspendall_prev_state));
	os_interrupt_disableall_enter(int_state);

}

FUNC(void, OS_CODE) os_enable_all_interrupts(void)
{
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_disableall(int_state) == FALSE)) {
		return;
	}
	os_interrupt_disableall_exit(int_state);
	os_int_resume_cat1(&(int_state->suspendall_prev_state));
}

FUNC(void, OS_CODE) os_suspend_all_interrupts(void)
{
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_disableall(int_state) == TRUE)) {
		return;
	}
	if (LIKELY(int_state->lock_state.suspendall_cnt == 0u)) {
		os_int_suspend_cat1(&int_state->suspendall_prev_state);
		int_state->suspendos_snapshot = int_state->lock_state.suspendos_cnt;
	}
	int_state->lock_state.suspendall_cnt++;
}

FUNC(void, OS_CODE) os_resume_all_interrupts(void)
{
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_suspendall(int_state) == FALSE)) {
		return;
	}
	int_state->lock_state.suspendall_cnt--;
	if (LIKELY(int_state->lock_state.suspendall_cnt == 0u)) {
		if (LIKELY(int_state->suspendos_snapshot ==
			   int_state->lock_state.suspendos_cnt)) {
			os_int_resume_cat1(&(int_state->suspendall_prev_state));
		}
	}
}

FUNC(void, OS_CODE) os_suspend_osinterrupts(void)
{
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_disableall(int_state) == TRUE)) {
		return;
	}
	if (LIKELY(int_state->lock_state.suspendos_cnt == 0u)) {
		os_int_suspend_cat2(&(int_state->suspendos_prev_state));
		int_state->suspendall_snapshot = int_state->lock_state.suspendall_cnt;
	}
	int_state->lock_state.suspendos_cnt++;
}

FUNC(void, OS_CODE) os_resume_osinterrupts(void)
{
	os_int_api_state_t *int_state = os_core_get_interrupt_api_stateref();

	if (UNLIKELY(os_interrupt_is_in_suspendos(int_state) == FALSE)) {
		return;
	}
	int_state->lock_state.suspendos_cnt--;
	if (LIKELY(int_state->lock_state.suspendos_cnt == 0u)) {
		if (LIKELY(int_state->suspendall_snapshot ==
			   int_state->lock_state.suspendall_cnt)) {
			os_int_resume_cat2(&(int_state->suspendos_prev_state));
		}
	}
}

FUNC(void, OS_CODE) os_interrupt_init(os_int_api_state_t *const int_state)
{
	if (int_state != NULL_PTR) {
		int_state->lock_state.in_disableall = (os_int_api_cnt_t)0;

		int_state->lock_state.suspendall_cnt = (os_int_api_cnt_t)0;

		int_state->lock_state.suspendos_cnt = (os_int_api_cnt_t)0;

		int_state->suspendall_prev_state.global = (os_int_global_state_t)0;

		int_state->suspendall_prev_state.level = (os_int_level_t)0;

		int_state->suspendos_prev_state.global = (os_int_global_state_t)0;

		int_state->suspendos_prev_state.level = (os_int_level_t)0;

		int_state->suspendall_snapshot = (os_int_api_cnt_t)0;

		int_state->suspendos_snapshot = (os_int_api_cnt_t)0;
	}
}

FUNC(boolean, OS_CODE) os_interrupt_is_disabled(void)
{
	return os_interrupt_is_in_critical_section(os_core_get_interrupt_api_stateref());
}

FUNC(void, OS_CODE) os_int_suspend_kernel(os_int_state_t *const int_state)
{
	int_state->global = up_irq_save();
}

FUNC(void, OS_CODE) os_int_resume_global(const os_int_state_t *const int_state)
{
	up_irq_restore(int_state->global);
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
static FUNC(Os_StatusType, OS_CODE) os_interrupt_access_check(const os_isr_cfg_t *IsrCfgRef)
{
	const os_ctx_cfg_t *const current_ctx = os_context_get_contexthdl();

	if (OS_STATUS_OK !=
	    os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_ctx), IsrCfgRef->context.access_app_mask)) {
		return OS_STATUS_ACCESSRIGHTS_1;
	} else {
		return OS_STATUS_OK;
	}
}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

FUNC(Os_StatusType, OS_CODE) os_enable_interrupt_source(isr_id_t isr_id, boolean clear_pending)
{
	Os_StatusType status = OS_STATUS_NOT_OK;

	/* Only if OsStatus is EXTENDED */
	if (isr_id >= OS_ISR_CNT) {
		status = OS_STATUS_ID_1;
	}
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	else if (os_app_check_is_accessible(GET_APP_CFG_BY_ISR_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_NOTACCESSIBLE_1;
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
			status = OS_STATUS_ACCESSRIGHTS_1;
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

FUNC(Os_StatusType, OS_CODE) os_disable_interrupt_source(isr_id_t isr_id)
{
	Os_StatusType status = OS_STATUS_NOT_OK;

	if (isr_id >= OS_ISR_CNT) {
		status = OS_STATUS_ID_1;
	}
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	else if (os_app_check_is_accessible(GET_APP_CFG_BY_ISR_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_NOTACCESSIBLE_1;
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
			status = OS_STATUS_ACCESSRIGHTS_1;
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

FUNC(Os_StatusType, OS_CODE) os_clear_pending_interrupt(isr_id_t isr_id)
{
	Os_StatusType status = OS_STATUS_NOT_OK;

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
		status = OS_STATUS_NOTACCESSIBLE_1;
	} else if (os_interrupt_access_check(OS_ISR_GET_ISRREF_BY_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESSRIGHTS_1;
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
#include "os_memmap.h"
