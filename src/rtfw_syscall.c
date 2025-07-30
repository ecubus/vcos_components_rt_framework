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
#include "rtfw_syscall_internal.h"
#include "rtfw_task.h"
#include "rtfw_event.h"
#include "rtfw_interrupt.h"
#include "rtfw_core.h"
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "rtfw_schedtable.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
#include "rtfw_spinlock.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
#include "rtfw_resource.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
#include "rtfw_counter.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
#include "rtfw_ioc.h"
#endif

/* global Function Implementation */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) rtfw_syscall_handler(void *syscall_args, uint32_t *regs)
{
	syscall_data_t *syscall;

	up_syscall_set_enter_int_state(regs);
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
		case OS_SYSCALLID_CHAINTASK:
			syscall->syscall_return_value.return_os_status =
				os_chain_task(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t);
			break;
		case OS_SYSCALLID_GETTASKID:
			syscall->syscall_return_value.return_os_status =
				os_get_task_id(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_ref);
			break;
		case OS_SYSCALLID_TASK_TICKDELAY:
			syscall->syscall_return_value.return_os_status =
			rtfw_task_tickdelay(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_tick_t);
			break;
		case OS_SYSCALLID_SET_USER_TO_PRIVILEGE:
			up_syscall_set_prev_privilege(regs, true);
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
		case OS_SYSCALLID_EVENT_TICKWAIT:
			syscall->syscall_return_value.return_os_status =
				rtfw_event_tickwait(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask,
					     syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_t);
			break;
		case OS_SYSCALLID_GETEVENT:
			syscall->syscall_return_value.return_os_status =
				os_get_event(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t,
					     syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask_ref);
			break;
		case OS_SYSCALLID_DISABLEALLINTERRUPTS:
			os_disable_all_interrupts();
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_ENABLEALLINTERRUPTS:
			os_enable_all_interrupts();
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_SUSPENDALLINTERRUPTS:
			os_suspend_all_interrupts();
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_RESUMEALLINTERRUPTS:
			os_resume_all_interrupts();
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_SUSPENDOSINTERRUPTS:
			os_suspend_os_interrupts();
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_RESUMEOSINTERRUPTS:
			os_resume_os_interrupts();
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_GETCOREID:
			syscall->syscall_return_value.return_coreid_type =
			os_get_core_id();
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
		case OS_SYSCALLID_SETSCHEDULETABLEAPPID:
			syscall->syscall_return_value.return_os_status = os_set_schedule_table_app_id(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_applicationid);
			break;
		case OS_SYSCALLID_CLEARSCHEDULETABLEAPPID:
			syscall->syscall_return_value.return_os_status = os_clear_schedule_table_app_id(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_applicationid);
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
		case OS_SYSCALLID_DEVIATESCHEDULETABLE:
			syscall->syscall_return_value.return_os_status = os_deviate_schedule_table(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_deviate);
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
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_RELEASESPINLOCK:
			syscall->syscall_return_value.return_os_status =
				os_release_spinlock(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_TRYTOGETSPINLOCK:
			syscall->syscall_return_value.return_os_status = os_try_to_get_spinlock(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_GETSPINLOCKREAD:
			syscall->syscall_return_value.return_os_status =
				os_get_spinlock_read(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_GETSPINLOCKWRITE:
			syscall->syscall_return_value.return_os_status =
				os_get_spinlock_write(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_RELEASESPINLOCKREAD:
			syscall->syscall_return_value.return_os_status =
				os_release_spinlock_read(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_RELEASESPINLOCKWRITE:
			syscall->syscall_return_value.return_os_status = os_release_spinlock_write(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_TRYTOGETSPINLOCKREAD:
			syscall->syscall_return_value.return_os_status = os_try_to_get_spinlock_read(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_TRYTOGETSPINLOCKWRITE:
			syscall->syscall_return_value.return_os_status = os_try_to_get_spinlock_write(
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
			up_syscall_set_prev_int_state(regs);
			break;
#endif /* CONFIG_RT_FRAMEWORK_SPINLOCK */
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
		case OS_SYSCALLID_GETRESOURCE:
			syscall->syscall_return_value.return_os_status =
				os_get_resource(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_resource);
			up_syscall_set_prev_int_state(regs);
			break;
		case OS_SYSCALLID_RELEASERESOURCE:
			syscall->syscall_return_value.return_os_status =
				os_release_resource(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_resource);
			up_syscall_set_prev_int_state(regs);
			break;
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
		case OS_SYSCALLID_GETCOUNTERVALUE:
			syscall->syscall_return_value.return_os_status =
				os_counter_get_value(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_ref_type);
			break;
		case OS_SYSCALLID_GETELAPSEDVALUE:
			syscall->syscall_return_value.return_os_status =
			os_counter_get_elapsed_value(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_ref_type,
				syscall->syscall_param[OS_SYSCALLPARAMINDEX_3].param_tick_ref_type);
			break;
		case OS_SYSCALLID_INCREMENTCOUNTER:
			syscall->syscall_return_value.return_os_status =
				os_counter_increment(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type);
			break;
#endif /* CONFIG_RT_FRAMEWORK_COUNTER */
#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)
		case OS_SYSCALLID_SERVICEERRORREPORT:
			vservice_error_report(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_service_id,
								  syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_os_status,
								  syscall->syscall_param[OS_SYSCALLPARAMINDEX_3].param_uint32_t,
								  syscall->syscall_param[OS_SYSCALLPARAMINDEX_4].param_va_list);
			break;
#endif /* CONFIG_RT_FRAMEWORK_ERROR */
#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
		case OS_SYSCALLID_IOC08SEND:
			syscall->syscall_return_value.return_os_status =
				os_ioc_08_send(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
							   syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint8_t);
			break;
		case OS_SYSCALLID_IOC08RECEIVE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_08_receive(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
								  syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint8_ref_t);
			break;
		case OS_SYSCALLID_IOC08EMPTYQUEUE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_08_empty_queue(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type);
			break;
		case OS_SYSCALLID_IOC16SEND:
			syscall->syscall_return_value.return_os_status =
				os_ioc_16_send(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
							   syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint16_t);
			break;
		case OS_SYSCALLID_IOC16RECEIVE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_16_receive(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
								  syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint16_ref_t);
			break;
		case OS_SYSCALLID_IOC16EMPTYQUEUE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_16_empty_queue(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type);
			break;
		case OS_SYSCALLID_IOC32SEND:
			syscall->syscall_return_value.return_os_status =
				os_ioc_32_send(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
							   syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t);
			break;
		case OS_SYSCALLID_IOC32RECEIVE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_32_receive(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
								  syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_ref_t);
			break;
		case OS_SYSCALLID_IOC32EMPTYQUEUE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_32_empty_queue(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type);
			break;
		case OS_SYSCALLID_IOCREFSEND:
			syscall->syscall_return_value.return_os_status =
				os_ioc_ref_send(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
								syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_const_void_ref_type);
			break;
		case OS_SYSCALLID_IOCREFRECEIVE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_ref_receive(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
								   syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_void_ref_type);
			break;
		case OS_SYSCALLID_IOCREFEMPTYQUEUE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_ref_empty_queue(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type);
			break;
		case OS_SYSCALLID_IOCWRITE:
			syscall->syscall_return_value.return_os_status =
				os_ioc_write(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
							 syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_const_void_ref_type);
			break;
		case OS_SYSCALLID_IOCREAD:
			syscall->syscall_return_value.return_os_status =
				os_ioc_read(syscall->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
							syscall->syscall_param[OS_SYSCALLPARAMINDEX_2].param_void_ref_type);
			break;
#endif /* CONFIG_RT_FRAMEWORK_IOC */
		default:
			/* do nothing */
			break;
	}

	up_syscall_set_exit_int_state(regs);
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
