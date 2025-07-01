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

#include <nuttx/config.h>
#include <sched/sched.h>
#include <nuttx/nuttx.h>
#include <nuttx/irq.h>
#include "rtfw_types.h"
#include "rtfw_hook.h"
#include "rtfw_task.h"
#include "rtfw_thread_internal.h"
#include "rtfw_core.h"
#include "setjmp.h"

static inline_function void os_hook_call(const os_hook_cfg_t *hook_cfg)
{
	const os_ctx_cfg_t *pre_context = os_context_get_contexthdl();
	os_context_id_t pre_context_id = OS_CONTEXT_GET_CURRENT_CONTEXTID(pre_context);
	const os_hook_cfg_t *pre_hook_cfg = NULL_PTR;
	os_hook_inst_t *pre_hook_inst = NULL_PTR;
	os_hook_inst_t *this_hook_inst = OS_HOOK_GET_INSTANCE_REF(hook_cfg);
	os_core_cfg_t *core = (os_core_cfg_t *)OS_HOOK_GET_CORE_REF(hook_cfg);
	uint32 hook_action = 0;

	irqstate_t irq_state = up_irq_save();

	if (pre_context_id & OS_CONTEXT_ANYHOOK) {
		pre_hook_cfg = OS_CONTEXT_GET_HOOK_CFG_REF(pre_context);
		pre_hook_inst = OS_HOOK_GET_INSTANCE_REF(pre_hook_cfg);
		if (((pre_hook_inst->accumulated_hook_ctx & hook_cfg->nesting_mask) == 0) ||
		    (pre_hook_inst->cur_nesting_depth >= OS_HOOK_MAX_NESTING)) {
			up_irq_restore(irq_state);
			return;
		} else {
			this_hook_inst->accumulated_hook_ctx |= pre_hook_inst->accumulated_hook_ctx;
			this_hook_inst->cur_nesting_depth = pre_hook_inst->cur_nesting_depth + 1;
		}
	}

	(void)os_context_nest_inc(OS_CORE_GET_CONTEXTNEST_REF(core), pre_context);

	hook_action = os_thread_get_hook_action(pre_context_id, TRUE);

	switch (hook_action) {
		case HOOK_CALL_FROM_TASK:
			up_switch_hook_context(hook_action, &(OS_CONTEXT_GET_TASK_INSTANCE_REF(pre_context)->tcb),
					       &(this_hook_inst->tcb));
			break;
		case HOOK_CALL_FROM_HOOK:
			up_switch_hook_context(hook_action, &(OS_CONTEXT_GET_HOOK_INSTANCE_REF(pre_context)->tcb),
					       &(this_hook_inst->tcb));
			break;
		case HOOK_CALL_FROM_IRQ:
			up_switch_hook_context(hook_action, &(this_hook_inst->saved_regs), &(this_hook_inst->tcb));
			break;
		default:
			break;
	}

	up_irq_restore(irq_state);
}

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) os_hook_init(const os_hook_cfg_t *const os_hook_cfg_table[], OsObjIndexType size)
{
	const os_hook_cfg_t *hook_cfg;
	os_hook_inst_t *hook_inst;
	OsObjIndexType index;
	const os_ctx_cfg_t *context_cfg;
	CoreIdType core_id = os_get_core_id();
	int ret;

	if ((os_hook_cfg_table == NULL) || (size == (OsObjIndexType)0)) {
		return;
	}

	for (index = 0; index < size; index++) {
		hook_cfg = os_hook_cfg_table[index];
		if (hook_cfg == NULL) {
			continue;
		}
		context_cfg = OS_HOOK_GET_CONTEXT_REF(hook_cfg);
		hook_inst = OS_HOOK_GET_INSTANCE_REF(hook_cfg);
		if ((hook_inst == NULL) || (OS_CONTEXT_GET_OWNER_COREID(context_cfg) != core_id)) {
			continue;
		}

		os_context_init_context_instance(context_cfg);

		hook_inst->accumulated_hook_ctx = hook_cfg->context.initial_ctx_id;
		hook_inst->saved_regs = NULL;
		hook_inst->cur_nesting_depth = 0;

		hook_inst->tcb.flags = TCB_FLAG_TTYPE_KERNEL | TCB_FLAG_HOOK_THREAD
								| TCB_FLAG_THREAD_DISABLE_IRQ
								| TCB_FLAG_THREAD_IN_KERNEL_MPU_SET;
		hook_inst->tcb.contexthdl = (void *)context_cfg;

#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
		up_mpu_enable_stack_region((uint32_t)(hook_cfg->context.arch_ctx_cfg.stack_addr),
					   (uint32_t)(hook_cfg->context.arch_ctx_cfg.stack_addr) +
						   hook_cfg->context.arch_ctx_cfg.stack_size);
#endif

		ret = nxtask_init((struct task_tcb_s *)&hook_inst->tcb, hook_cfg->context.instance_name, 0,
				  (void *)hook_cfg->context.arch_ctx_cfg.stack_addr,
				  hook_cfg->context.arch_ctx_cfg.stack_size, NULL, NULL, NULL, NULL);
		if (ret < OK) {
			continue;
		}
	}
}

FUNC(Os_StatusType, OS_CODE) os_hook_call_void_hook(HookType hook_id)
{
	const os_hook_cfg_t *hook_cfg = os_hook_cfg_ref_table[hook_id];

	if ((hook_cfg == NULL) || (OS_HOOK_GET_INSTANCE_REF(hook_cfg) == NULL_PTR)) {
		return OS_STATUS_PARAM_POINTER_1;
	}

	os_hook_call(hook_cfg);

	return OS_STATUS_OK;
}

FUNC(Os_StatusType, OS_CODE) os_hook_call_status_hook(HookType hook_id, StatusType error)
{
	const os_hook_cfg_t *hook_cfg = os_hook_cfg_ref_table[hook_id];
	os_hook_inst_t *hook_inst = NULL_PTR;

	if ((hook_cfg == NULL) || (OS_HOOK_GET_INSTANCE_REF(hook_cfg) == NULL_PTR)) {
		return OS_STATUS_PARAM_POINTER_1;
	}

	hook_inst = OS_HOOK_GET_INSTANCE_REF(hook_cfg);
	hook_inst->parameter.status = error;
	os_hook_call(hook_cfg);
	return OS_STATUS_OK;
}

FUNC(protection_return_t, OS_CODE) os_hook_call_protection_hook(HookType hook_id, StatusType error)
{
	const os_hook_cfg_t *hook_cfg = os_hook_cfg_ref_table[hook_id];
	os_hook_inst_t *hook_inst = NULL_PTR;

	if ((hook_cfg == NULL) || (OS_HOOK_GET_INSTANCE_REF(hook_cfg) == NULL_PTR)) {
		return PRO_IGNORE;
	}

	hook_inst = OS_HOOK_GET_INSTANCE_REF(hook_cfg);
	hook_inst->parameter.status = error;
	os_hook_call(hook_cfg);

	return hook_inst->return_value.protection_reaction;
}

#if defined(CONFIG_RT_FRAMEWORK_ERROR_HOOK) && (CONFIG_RT_FRAMEWORK_ERROR_HOOK == 1)
FUNC(void, OS_CODE) os_call_error_hook(const os_core_cfg_t *core_cfg, StatusType error_status)
{
	const os_hook_cfg_t *error_hook;
	const os_ctx_cfg_t *context = os_context_get_contexthdl();
	const os_app_cfg_t *app_cfg = OS_CONTEXT_GET_CURRENTAPPCFG_REF(context);
	const os_context_id_t cur_ctx_id = OS_CONTEXT_GET_CURRENT_CONTEXTID(context);

	if ((cur_ctx_id & (OS_PRETASKHOOK_CONTEXT_ID | OS_POSTTASKHOOK_CONTEXT_ID)) == 0u) {
		error_hook = OS_CORE_GET_ERROR_HOOK(core_cfg);
		if (error_hook != NULL_PTR) {
			(void)os_hook_call_status_hook(OS_HOOK_GET_HOOKID(error_hook), error_status);
		}

		error_hook = app_cfg->error_hook;
		if (error_hook != NULL_PTR) {
			(void)os_hook_call_status_hook(OS_HOOK_GET_HOOKID(error_hook), error_status);
		}
	}
}
#endif

#if defined(CONFIG_RT_FRAMEWORK_STARTUP_HOOK) && (CONFIG_RT_FRAMEWORK_STARTUP_HOOK == 1)
FUNC(void, OS_CODE) os_call_startup_hook(const os_core_cfg_t *core_cfg)
{
	OsObjIndexType app_index;
	const os_hook_cfg_t *startup_hook;

	startup_hook = OS_CORE_GET_STARTUP_HOOK(core_cfg);
	if (startup_hook != NULL_PTR) {
		(void)os_hook_call_void_hook(OS_HOOK_GET_HOOKID(startup_hook));
	}

	for (app_index = 0; app_index < (*(core_cfg->app_count)); ++app_index) {
		startup_hook = (core_cfg->app_refs[app_index])->startup_hook;
		if (startup_hook != NULL_PTR) {
			(void)os_hook_call_void_hook(OS_HOOK_GET_HOOKID(startup_hook));
		}
	}
}
#endif

#if defined(CONFIG_RT_FRAMEWORK_PROTECTION_HOOK) && (CONFIG_RT_FRAMEWORK_PROTECTION_HOOK == 1)
FUNC(protection_return_t, OS_CODE) os_call_protection_hook(StatusType error_status)
{
	protection_return_t response = OS_PROTECTION_REACTION_COUNT;

	const os_ctx_cfg_t *context = os_context_get_contexthdl();
	const os_hook_cfg_t *hook_cfg = OS_CORE_GET_PROTECTION_HOOK(OS_CONTEXT_GET_CORE_REF(context));

	if (hook_cfg != NULL_PTR) {
		response = os_hook_call_protection_hook(hook_cfg->hook_id, error_status);
	}
	return response;
}
#endif

#if defined(CONFIG_RT_FRAMEWORK_PRETASK_HOOK) && (CONFIG_RT_FRAMEWORK_PRETASK_HOOK == 1)
FUNC(void, OS_CODE) rtfw_call_pretask_hook(void)
{
	os_ctx_inst_t *context_inst;
	os_context_id_t saved_context_id;
	const os_ctx_cfg_t *context = os_context_get_contexthdl();

	if (context != NULL_PTR) {
		context_inst = OS_CONTEXT_GET_CONTEXTINST_REF(context);
		saved_context_id = OS_CONTEXTINST_GET_CURRENT_CONTEXTID(context_inst);

		OS_CONTEXTINST_GET_CURRENT_CONTEXTID(context_inst) = OS_PRETASKHOOK_CONTEXT_ID;

		os_pretaskhook();

		OS_CONTEXTINST_GET_CURRENT_CONTEXTID(context_inst) = saved_context_id;
	}
}

#endif

#if defined(CONFIG_RT_FRAMEWORK_POSTTASK_HOOK) && (CONFIG_RT_FRAMEWORK_POSTTASK_HOOK == 1)
FUNC(void, OS_CODE) rtfw_call_posttask_hook(void)
{
	os_ctx_inst_t *context_inst;
	os_context_id_t saved_context_id;
	const os_ctx_cfg_t *context = os_context_get_contexthdl();

	if (context != NULL_PTR) {
		context_inst = OS_CONTEXT_GET_CONTEXTINST_REF(context);
		saved_context_id = OS_CONTEXTINST_GET_CURRENT_CONTEXTID(context_inst);

		OS_CONTEXTINST_GET_CURRENT_CONTEXTID(context_inst) = OS_POSTTASKHOOK_CONTEXT_ID;

		os_posttaskhook();

		OS_CONTEXTINST_GET_CURRENT_CONTEXTID(context_inst) = saved_context_id;
	}
}

#endif

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
