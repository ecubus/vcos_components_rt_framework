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
#include <nuttx/irq.h>
#include "os_syscall_internal.h"
#include "os_task.h"
#include "os_event.h"
#include "os_interrupt.h"
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "os_schedtable.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
#include "os_spinlock.h"
#endif

/* global Function Implementation */
#define OS_START_SEC_CODE
#include "os_memmap.h"

FUNC(void, OS_CODE) syscall_handler(void *syscall_args, uint32_t *regs)
{
	syscall_data_t *syscall;

	up_set_int_stack(regs);
	syscall = (syscall_data_t *)syscall_args;

	switch (syscall->syscall_id) {
		case OS_SYSCALLID_ACTIVATETASK:
			syscall->syscall_return_value.return_os_status =
				os_activate_task(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t);
			break;
		case OS_SYSCALLID_ACTIVATETASKASYNC:
			syscall->syscall_return_value.return_os_status =
				os_activate_task_asyn(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t);
			break;
		case OS_SYSCALLID_SET_USER_TO_PRIVILEGE:
			up_set_user_to_privilege(regs);
			break;
		case OS_SYSCALLID_SETEVENT:
			syscall->syscall_return_value.return_os_status =
				os_set_event(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t,
					     syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask);
			break;
		case OS_SYSCALLID_SETEVENTASYN:
			syscall->syscall_return_value.return_os_status =
				os_set_event_asyn(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t,
						  syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask);
			break;
		case OS_SYSCALLID_CLEAREVENT:
			syscall->syscall_return_value.return_os_status =
				os_clear_event(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask);
			break;
		case OS_SYSCALLID_WAITEVENT:
			syscall->syscall_return_value.return_os_status =
				os_wait_event(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask);
			break;
		case OS_SYSCALLID_GETEVENT:
			syscall->syscall_return_value.return_os_status =
				os_get_event(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t,
					     syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask_ref);
			break;
		case OS_SYSCALLID_DISABLEALLINTERRUPTS:
			os_disable_all_interrupts();
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_ENABLEALLINTERRUPTS:
			os_enable_all_interrupts();
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_SUSPENDALLINTERRUPTS:
			os_suspend_all_interrupts();
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_RESUMEALLINTERRUPTS:
			os_resume_all_interrupts();
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_SUSPENDOSINTERRUPTS:
			os_suspend_osinterrupts();
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_RESUMEOSINTERRUPTS:
			os_resume_osinterrupts();
			up_set_int_from_syscall(regs);
			break;
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
		case OS_SYSCALLID_STARTSCHEDULETABLEREL:
			syscall->syscall_return_value.return_os_status = os_start_schedule_table_rel(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t);
			break;
		case OS_SYSCALLID_STARTSCHEDULETABLEABS:
			syscall->syscall_return_value.return_os_status = os_start_schedule_table_abs(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t);
			break;
		case OS_SYSCALLID_STOPSCHEDULETABLE:
			syscall->syscall_return_value.return_os_status = os_stop_schedule_table(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable);
			break;
		case OS_SYSCALLID_GETSCHEDULETABLESTATUS:
			syscall->syscall_return_value.return_os_status = os_get_schedule_table_status(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable_status_ref);
			break;
		case OS_SYSCALLID_NEXTSCHEDULETABLE:
			syscall->syscall_return_value.return_os_status = os_next_schedule_table(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable);
			break;
		case OS_SYSCALLID_NEXTSCHEDULETABLEFROMSTOP:
			syscall->syscall_return_value.return_os_status = os_next_schedule_table_from_stop(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable);
			break;
		case OS_SYSCALLID_STARTSCHEDULETABLESYNCHRON:
			syscall->syscall_return_value.return_os_status = os_start_schedule_table_synchron(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable);
			break;
		case OS_SYSCALLID_SYNCSCHEDULETABLE:
			syscall->syscall_return_value.return_os_status = os_sync_schedule_table(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_t);
			break;
		case OS_SYSCALLID_SETSCHEDULETABLEASYNC:
			syscall->syscall_return_value.return_os_status = os_set_schedule_table_async(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable);
			break;
#endif /* CONFIG_RT_FRAMEWORK_SCHTBL */
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
		case OS_SYSCALLID_GETSPINLOCK:
			syscall->syscall_return_value.return_os_status =
				os_get_spinlock(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_RELEASESPINLOCK:
			syscall->syscall_return_value.return_os_status =
				os_release_spinlock(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_TRYTOGETSPINLOCK:
			syscall->syscall_return_value.return_os_status = os_try_to_get_spinlock(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_GETSPINLOCKREAD:
			syscall->syscall_return_value.return_os_status =
				os_get_spinlock_read(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_GETSPINLOCKWRITE:
			syscall->syscall_return_value.return_os_status = os_get_spinlock_write(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_RELEASESPINLOCKREAD:
			syscall->syscall_return_value.return_os_status = os_release_spinlock_read(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_RELEASESPINLOCKWRITE:
			syscall->syscall_return_value.return_os_status = os_release_spinlock_write(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_TRYTOGETSPINLOCKREAD:
			syscall->syscall_return_value.return_os_status = os_try_to_get_spinlock_read(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
			up_set_int_from_syscall(regs);
			break;
		case OS_SYSCALLID_TRYTOGETSPINLOCKWRITE:
			syscall->syscall_return_value.return_os_status = os_try_to_get_spinlock_write(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
			up_set_int_from_syscall(regs);
			break;
#endif /* CONFIG_RT_FRAMEWORK_SPINLOCK */
		default:
			/* do nothing */
			break;
	}

	up_restore_int_stack(regs);
}

#define OS_STOP_SEC_CODE
#include "os_memmap.h"
