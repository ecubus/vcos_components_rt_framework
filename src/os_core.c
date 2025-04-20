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
#include "nuttx/config.h"
#include "os_atomic.h"
#include "os_core.h"
#include "core_cfg.h"

#define OS_CORE_SYNC_MASK ((0x1U << (CONFIG_RT_FRAMEWORK_NCORES)) - 1U)
#define OS_CORE_SYNC_FLAG ((0x1U << (CONFIG_RT_FRAMEWORK_NCORES)) - 1U)

/* TODO: if cache is enabled, these two variables should be mapped to none cache area*/
Atomic g_coresync_flag_one;
Atomic g_coresync_flag_two;

static inline void os_core_sync_all_cores(Atomic *const sync_value_ref)
{
	sint32 core_id = (sint32)os_get_core_id();
	(void)Os_AtomicOr(sync_value_ref, (sint32)(1U << core_id));

	while (((*sync_value_ref) & OS_CORE_SYNC_MASK) != OS_CORE_SYNC_FLAG) {
	}
}

#define OS_START_SEC_CODE
#include "os_memmap.h"

FUNC(void, OS_CODE) os_core_sync_phase_one(void)
{
	os_core_sync_all_cores(&g_coresync_flag_one);
}

FUNC(void, OS_CODE) os_core_sync_phase_two(void)
{
	os_core_sync_all_cores(&g_coresync_flag_two);
}

#define OS_STOP_SEC_CODE
#include "os_memmap.h"
