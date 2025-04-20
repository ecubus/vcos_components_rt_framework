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
#include "Os.h"
#include "os_core.h"
#include "os_task.h"
#include "os_event.h"
#include "os_interrupt.h"
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "os_schedtable.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
#include "os_spinlock.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
#include "os_syscall_internal.h"
#endif

#define OS_START_SEC_CODE
#include "os_memmap.h"

FUNC(CoreIdType, OS_CODE) GetCoreID(void)
{
	return os_get_core_id();
}

FUNC(StatusType, OS_CODE) ActivateTask(TaskType task_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_activate_task(task_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_ACTIVATETASK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(void, OS_CODE) ActivateTaskAsyn(TaskType task_id)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		(void)os_activate_task_asyn(task_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_ACTIVATETASKASYNC;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;

		up_syscall(&syscall);
	}
#endif
}

FUNC(StatusType, OS_CODE) TerminateTask(void)
{
	Os_StatusType status;

	status = os_terminate_task();

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) WaitEvent(EventMaskType mask)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_wait_event(mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_WAITEVENT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask = mask;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) SetEvent(TaskType task_id, EventMaskType mask)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_set_event(task_id, mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SETEVENT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask = mask;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(void, OS_CODE) SetEventAsyn(TaskType task_id, EventMaskType mask)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		(void)os_set_event_asyn(task_id, mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SETEVENTASYN;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask = mask;

		up_syscall(&syscall);
	}
#endif
}

FUNC(StatusType, OS_CODE) GetEvent(TaskType task_id, EventMaskRefType mask)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_get_event(task_id, mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETEVENT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask_ref = mask;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ClearEvent(EventMaskType mask)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_clear_event(mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_CLEAREVENT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask = mask;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
FUNC(StatusType, OS_CODE) StartScheduleTableRel(ScheduleTableType schedtable_id, TickType Offset)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_start_schedule_table_rel(schedtable_id, Offset);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_STARTSCHEDULETABLEREL;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t = Offset;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) StartScheduleTableAbs(ScheduleTableType schedtable_id, TickType start)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_start_schedule_table_abs(schedtable_id, start);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_STARTSCHEDULETABLEABS;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t = start;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) StartScheduleTableSynchron(ScheduleTableType schedtable_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_start_schedule_table_synchron(schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_STARTSCHEDULETABLESYNCHRON;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) StopScheduleTable(ScheduleTableType schedtable_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_stop_schedule_table(schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_STOPSCHEDULETABLE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) NextScheduleTable(ScheduleTableType curr_schedtable_id, ScheduleTableType next_schedtable_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_next_schedule_table(curr_schedtable_id, next_schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_NEXTSCHEDULETABLE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = curr_schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable = next_schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE)
NextScheduleTableFromStop(ScheduleTableType curr_schedtable_id, ScheduleTableType next_schedtable_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_next_schedule_table_from_stop(curr_schedtable_id, next_schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_NEXTSCHEDULETABLEFROMSTOP;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = curr_schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable = next_schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) SyncScheduleTable(ScheduleTableType schedtable_id, TickType tick_value)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_sync_schedule_table(schedtable_id, tick_value);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SYNCSCHEDULETABLE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_t = tick_value;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetScheduleTableStatus(ScheduleTableType schedtable_id, ScheduleTableStatusRefType StatusRef)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_get_schedule_table_status(schedtable_id, StatusRef);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETSCHEDULETABLESTATUS;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable_status_ref = StatusRef;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) SetScheduleTableAsync(ScheduleTableType schedtable_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_set_schedule_table_async(schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SETSCHEDULETABLEASYNC;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	return (StatusType)status;
}

#endif /* CONFIG_RT_FRAMEWORK_SCHTBL */

#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
FUNC(StatusType, OS_CODE) GetSpinlock(SpinlockIdType spinlock_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_get_spinlock(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_GETSPINLOCK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ReleaseSpinlock(SpinlockIdType spinlock_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_release_spinlock(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_RELEASESPINLOCK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) TryToGetSpinlock(SpinlockIdType spinlock_id, TryToGetSpinlockType *success)
{
	Os_StatusType status;
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_try_to_get_spinlock(spinlock_id, success);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_TRYTOGETSPINLOCK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref = success;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetSpinlockRead(SpinlockIdType spinlock_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_get_spinlock_read(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_GETSPINLOCKREAD;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetSpinlockWrite(SpinlockIdType spinlock_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_get_spinlock_write(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_GETSPINLOCKWRITE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ReleaseSpinlockRead(SpinlockIdType spinlock_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_release_spinlock_read(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_RELEASESPINLOCKREAD;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ReleaseSpinlockWrite(SpinlockIdType spinlock_id)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_release_spinlock_write(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_RELEASESPINLOCKWRITE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) TryToGetSpinlockRead(SpinlockIdType spinlock_id, TryToGetSpinlockType *success)
{
	Os_StatusType status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_try_to_get_spinlock_read(spinlock_id, success);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_TRYTOGETSPINLOCKREAD;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref = success;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) TryToGetSpinlockWrite(SpinlockIdType spinlock_id, TryToGetSpinlockType *success)
{
	Os_StatusType status;
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		status = os_try_to_get_spinlock_write(spinlock_id, success);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_TRYTOGETSPINLOCKWRITE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref = success;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	return (StatusType)status;
}

#endif /* CONFIG_RT_FRAMEWORK_SPINLOCK */

FUNC(void, OS_CODE) DisableAllInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		os_disable_all_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_DISABLEALLINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) EnableAllInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		os_enable_all_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_ENABLEALLINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) SuspendAllInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		os_suspend_all_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_SUSPENDALLINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) ResumeAllInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		os_resume_all_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_RESUMEALLINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) SuspendOSInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		os_suspend_osinterrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_SUSPENDOSINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) ResumeOSInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	if (TRUE == is_privilege_mode()) {
#endif
		os_resume_osinterrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL== 1)
	} else {
		syscall_data_t syscall;
		syscall.syscall_id = OS_SYSCALLID_RESUMEOSINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

#define OS_STOP_SEC_CODE
#include "os_memmap.h"
