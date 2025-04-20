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
#include <nuttx/event.h>
#include <nuttx/signal.h>
#include <nuttx/sched_note.h>

#include <sched/sched.h>
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
#include "os_ipicall.h"
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
#include "os_task.h"
#include "os_core.h"
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
#include "os_syscall_internal.h"
#endif
#include "setjmp.h"

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
static inline_function void os_set_task_mode_to_privilege(void)
{
	syscall_data_t syscall;

	if (TRUE != is_privilege_mode()) {
		syscall.syscall_id = OS_SYSCALLID_SET_USER_TO_PRIVILEGE;
		up_syscall(&syscall);
	}
}
static inline_function void os_set_task_mode_from_privilege(bool is_privilege)
{
	if (TRUE != is_privilege){
		up_set_privilege_to_user();
	}
}
#endif

#define OS_START_SEC_CODE
#include "os_memmap.h"

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
static noinline_function FUNC(void, OS_CODE) os_call_task_function(const os_task_cfg_t *task_cfg, boolean is_privilege)
{
	os_set_task_mode_from_privilege(is_privilege);
	task_cfg->context.arch_ctx_cfg.entry_addr(task_cfg->context.arch_ctx_cfg.entry_param);
}
#endif

static FUNC(int, OS_CODE) os_prestart_task(int argc, FAR char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	(void)up_irq_save();
	this_task()->xcp.task_started = 1;
	os_context_set_contexthdl((const os_ctx_cfg_t *const)(this_task()->xcp.contexthdl));
	const os_ctx_cfg_t *current_context = (const os_ctx_cfg_t *)(this_task()->xcp.contexthdl);
	const os_task_cfg_t *task_cfg = OS_CONTEXT_GET_TASK_CFG_REF(current_context);
	os_task_inst_t *task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(current_context);

	if (setjmp(task_inst->jmpbuf) == 1) {
#if (CONFIG_RT_FRAMEWORK_TASK_MULTIPLE_ACTIVATION != 1)
		nxsem_reset(&task_inst->act_sem, 0);
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION
		if (task_inst->act_sem.semcount >= 1) {
			sched_note_reactivate(&task_inst->tcb);
		}
#endif
	}
	up_irq_enable();

	nxsem_wait_uninterruptible(&(task_inst->act_sem));

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	os_call_task_function(task_cfg, current_context->app_owner_cfg->IsPrivileged);
#else
	task_cfg->context.arch_ctx_cfg.entry_addr(task_cfg->context.arch_ctx_cfg.entry_param);
#endif

	while (1) {
		/* It is illegal to get here !!!*/
		nxsig_usleep(MAX_DELAY);
	}

	return OK;
}

FUNC(void, OS_CODE) os_task_init(const os_task_cfg_t *const os_task_cfg_table[], OsObjIndexType size)
{
	const os_task_cfg_t *task_cfg;
	os_task_inst_t *task_inst;
	OsObjIndexType index;
	const os_ctx_cfg_t *context_cfg;
	CoreIdType core_id = os_get_core_id();
	int ret;

	if ((os_task_cfg_table == NULL) || (size == (OsObjIndexType)0)) {
		return;
	}

	for (index = 0; index < size; index++) {
		task_cfg = os_task_cfg_table[index];
		if ((task_cfg == NULL)) {
			continue;
		}
		context_cfg = OS_TASK_GET_CONTEXT_REF(task_cfg);
		task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(context_cfg);
		if ((task_inst == NULL) || (OS_CONTEXT_GET_OWNER_COREID(context_cfg) != core_id)) {
			continue;
		}
		nxsem_init(&(task_inst->act_sem), 0, 0);

		task_inst->tcb.flags = TCB_FLAG_TTYPE_KERNEL;

		ret = nxtask_init((struct task_tcb_s *)&task_inst->tcb, task_cfg->context.instance_name,
				  (int)task_cfg->base_priority, (void *)task_cfg->context.arch_ctx_cfg.stack_addr,
				  task_cfg->context.arch_ctx_cfg.stack_size, os_prestart_task, NULL, NULL, NULL);
		if (ret < OK) {
			continue;
		}

		task_inst->tcb.xcp.contexthdl = (void *)context_cfg;
		task_inst->tcb.xcp.task_started = 0;
		os_context_init_context_instance(context_cfg);
		if (task_cfg->is_extended) {
			nxevent_init(&(task_inst->event), 0);
		}
		if (task_cfg->autostart_mode) {
			nxsem_post(&(task_inst->act_sem));
		}
		nxtask_activate(&task_inst->tcb);
	}
}

FUNC(Os_StatusType, OS_CODE) os_activate_task(TaskType task_id)
{
	const os_task_cfg_t *task_cfg = os_task_cfg_ref_table[task_id];
	os_task_inst_t *task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);
	const os_ctx_cfg_t *task_context = OS_TASK_GET_CONTEXT_REF(task_cfg);
	CoreIdType task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);

	if (task_core_id == os_get_core_id()) {
		nxsem_post(&task_inst->act_sem);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_activate_task((uint32)task_core_id, task_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
	return OS_STATUS_OK;
}

FUNC(Os_StatusType, OS_CODE) os_terminate_task(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	os_set_task_mode_to_privilege();
#endif
	(void)up_irq_save();
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
	os_task_inst_t *task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(current_context);

	longjmp(task_inst->jmpbuf, 1);
	/* not be executed */
	return OS_STATUS_OK;
}

//TODO
FUNC(Os_StatusType, OS_CODE) os_activate_task_asyn(TaskType task_id)
{
	const os_task_cfg_t *task_cfg = os_task_cfg_ref_table[task_id];
	const os_ctx_cfg_t *task_context = OS_TASK_GET_CONTEXT_REF(task_cfg);
	CoreIdType task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);
	os_task_inst_t *task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);

	if (task_core_id == os_get_core_id()) {
		nxsem_post(&task_inst->act_sem);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_activate_task_asyn((uint32)task_core_id, task_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
	return OS_STATUS_OK;
}

FUNC(Os_StatusType, OS_CODE) get_task_app_id(ApplicationIDType *ApplicationIDRef)
{
	Os_StatusType ret = OS_STATUS_OK;
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();

	if (ApplicationIDRef != NULL_PTR) {
		*ApplicationIDRef = OS_CONTEXT_GET_CURRENT_APPID(current_context);
	} else {
		ret = OS_STATUS_PARAM_POINTER_2;
	}

	return ret;
}

#define OS_STOP_SEC_CODE
#include "os_memmap.h"
