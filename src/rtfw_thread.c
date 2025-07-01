#include <nuttx/config.h>
#include <nuttx/signal.h>
#include <nuttx/irq.h>
#include <nuttx/sched_note.h>
#include <task/task.h>
#include <sched/sched.h>
#include "setjmp.h"
#include "rtfw_thread_internal.h"
#include "rtfw_task.h"
#include "rtfw_hook.h"
#include "rtfw_core.h"
#include "rtfw_context.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

static noinline_function FUNC(void, OS_CODE) os_thread_enter_task(const os_task_cfg_t *task_cfg, boolean is_privilege)
{
	OS_TASK_GET_INSTANCE_REF(task_cfg)->tcb.flags &= ~TCB_FLAG_THREAD_IN_KERNEL_MPU_SET;
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	up_mpu_enable_set(OS_TASK_GET_CONTEXTINST_REF(task_cfg)->current_mpu_access_right->set_num);
#endif
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	os_set_thread_mode_from_privilege(is_privilege);
#else
	(void)is_privilege;
#endif
	task_cfg->context.arch_ctx_cfg.entry_addr(task_cfg->context.arch_ctx_cfg.entry_param);
}

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
static noinline_function FUNC(void, OS_CODE) os_thread_enter_hook(const os_hook_cfg_t *hook_cfg, boolean is_privilege)
{
	const os_ctx_cfg_t *this_context_cfg = OS_HOOK_GET_CONTEXT_REF(hook_cfg);
	os_hook_inst_t *hook_inst = OS_CONTEXT_GET_HOOK_INSTANCE_REF(this_context_cfg);
	os_context_id_t this_context_id = OS_CONTEXT_GET_INITCONTEXTID(this_context_cfg);
	void_hook_callback_t hook_call_back = hook_cfg->callback;
	StatusType hook_status = hook_inst->parameter.status;
	protection_return_t protection_return = PRO_IGNORE;

	OS_HOOK_GET_INSTANCE_REF(hook_cfg)->tcb.flags &= ~TCB_FLAG_THREAD_IN_KERNEL_MPU_SET;
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	up_mpu_enable_set(OS_HOOK_GET_CONTEXTINST_REF(hook_cfg)->current_mpu_access_right->set_num);
#endif
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	os_set_thread_mode_from_privilege(is_privilege);
#else
	(void)is_privilege;
#endif
	switch (this_context_id) {
		case OS_SHUTDOWNHOOK_CONTEXT_ID:
		case OS_ERRORHOOK_CONTEXT_ID:
			((status_hook_callback_t)hook_call_back)(hook_status);
			break;
		case OS_STARTUPHOOK_CONTEXT_ID:
			((void_hook_callback_t)hook_call_back)();
			break;
		case OS_PROTECTIONHOOK_CONTEXT_ID:
			protection_return = ((protection_hook_callback_t)hook_call_back)(hook_status);
			break;
		default:
			break;
	}
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	os_set_thread_mode_to_privilege();
#endif
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	up_mpu_enable_set(0U);
#endif
	hook_inst->return_value.protection_reaction = protection_return;
	longjmp(hook_inst->jmpbuf, 1);
}
#endif

static noinline_function void os_task_thread_start(int argc, FAR char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	up_irq_disable();
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
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

	os_thread_enter_task(task_cfg, current_context->app_owner_cfg->is_privileged);

	while (1) {
		/* It is illegal to get here !!!*/
		nxsig_usleep(MAX_DELAY);
	}
}

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
static noinline_function void os_hook_thread_start(int argc, FAR char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	const os_ctx_cfg_t *pre_context = NULL_PTR;
	os_context_id_t pre_context_id = 0;
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
	const os_hook_cfg_t *hook_cfg = OS_CONTEXT_GET_HOOK_CFG_REF(current_context);
	os_hook_inst_t *hook_inst = OS_CONTEXT_GET_HOOK_INSTANCE_REF(current_context);
	os_core_cfg_t *core = (os_core_cfg_t *)OS_HOOK_GET_CORE_REF(hook_cfg);
	uint32 hook_action = 0;

	if (setjmp(hook_inst->jmpbuf) == 1) {
		hook_inst->accumulated_hook_ctx = hook_cfg->context.initial_ctx_id;
		hook_inst->cur_nesting_depth = 0;

		pre_context = os_context_nest_dec(OS_CORE_GET_CONTEXTNEST_REF(core), NULL_PTR);
		pre_context_id = OS_CONTEXT_GET_CURRENT_CONTEXTID(pre_context);

		hook_action = os_thread_get_hook_action(pre_context_id, FALSE);

		switch (hook_action) {
			case HOOK_RETURN_TO_HOOK:
				up_switch_hook_context(hook_action, &(hook_inst->tcb),
						       &(OS_CONTEXT_GET_HOOK_INSTANCE_REF(pre_context)->tcb));
				break;
			case HOOK_RETURN_TO_TASK:
				up_switch_hook_context(hook_action, &(hook_inst->tcb),
						       &(OS_CONTEXT_GET_TASK_INSTANCE_REF(pre_context)->tcb));
				break;
			case HOOK_RETURN_TO_IRQ:
				os_context_set_contexthdl(pre_context);
				up_switch_hook_context(hook_action, &(hook_inst->tcb), hook_inst->saved_regs);
				break;
			default:
				break;
		}
	}

	os_thread_enter_hook(hook_cfg, current_context->app_owner_cfg->is_privileged);
}
#endif

void rtfw_thread_start(void)
{
	char **argv;
	int argc;
	struct tcb_s *tcb = NULL_PTR;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	os_context_id_t cur_context_id = OS_CONTEXT_GET_INITCONTEXTID(cur_context);
	boolean this_is_task = ((cur_context_id & OS_CONTEXT_ANYTASK) ? TRUE : FALSE);

	DEBUGASSERT((cur_context_id & OS_CONTEXT_ANYTASK) || (cur_context_id & OS_CONTEXT_ANYHOOK));

	if (this_is_task) {
		tcb = &(OS_CONTEXT_GET_TASK_INSTANCE_REF(cur_context)->tcb);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
		tcb = &(OS_CONTEXT_GET_HOOK_INSTANCE_REF(cur_context)->tcb);
#endif
	}

	argv = nxsched_get_stackargs(tcb);
	for (argc = 0; argv && argv[argc]; argc++) {
		/* do nothing */
	}

	if (this_is_task) {
		os_task_thread_start(argc, argv);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
		os_hook_thread_start(argc, argv);
#endif
	}
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"