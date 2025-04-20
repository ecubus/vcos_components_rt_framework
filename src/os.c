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
#include "Os_Types.h"
#include "os_task.h"
#include "os_isr.h"
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
#include "os_ipicall.h"
#endif
#include "os_counter.h"
#include "os_application.h"
#include "syslog.h"
#include "os_core.h"
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "os_schedtable.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
#include "os_spinlock.h"
#endif

#define OS_START_SEC_CODE
#include "os_memmap.h"

FUNC(void, OS_CODE) StartOS(void)
{
	CoreIdType core_id = os_get_core_id();
	const os_core_cfg_t *core = os_core_cfg_ref_table[core_id];
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
	if ((core->app_refs != NULL_PTR) && (core->app_count != NULL_PTR)) {
		os_app_init(core->app_refs, *(core->app_count));
	}
#else
	os_task_init(os_task_cfg_ref_table, os_task_cfg_ref_table_size);
	os_isr_init(os_isr_cfg_ref_table, os_isr_cfg_ref_table_size);
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
	os_counter_init(os_counter_cfg_ref_table, os_counter_cfg_ref_table_size);
	os_counter_start(os_counter_cfg_ref_table, os_counter_cfg_ref_table_size);
#endif
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	os_schedule_table_init(os_schedtable_cfg_ref_table, os_schedtable_cfg_ref_table_size);
#endif
#endif /* CONFIG_RT_FRAMEWORK_MULTIAPP */

#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
	os_ipicall_init(os_ipicall_cfg_ref_table, os_ipicall_cfg_ref_table_size);
#endif

#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	if (os_get_core_id() == 0) {
		os_spinlock_init();
	}
#endif
	os_core_sync_phase_one();
}

#define OS_STOP_SEC_CODE
#include "os_memmap.h"
