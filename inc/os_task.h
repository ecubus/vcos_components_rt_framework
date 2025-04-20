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

#ifndef AUTOSAROS_INC_OS_TASK_H
#define AUTOSAROS_INC_OS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "os_task_internal.h"

/* Global Enumeration Definition */

/* Global Type & Structure Definition */

/*! Task Instance Description */

/* Global Variable Declaration */

/* Global Function Declaration */

/* Global Function Implementation */

/* Global Macro Definition */
/*! Task stack declare */

#define STACKSIZE_ROUND_UP(size, data_type) (((size) + sizeof(data_type) - 1) & (~(sizeof(data_type) - 1)))

#define DeclareTaskStack(section, task_id, byte_size)                                                                   \
	OS_TABLE_DECLARE(section, OsStack, StackType_t, task_id, STACKSIZE_ROUND_UP(byte_size, StackType_t))

/*! Task declare */
#define DeclareBasicTask(section, task_id)                                                                              \
	OS_LINK_ENTRY_DECLARE(section, os_obj, os_task_inst_t, task_id);                                                 \
	OS_LINK_ENTRY_DECLARE(section, os_obj_context, os_ctx_inst_t, task_id);                                       \
	OS_LINK_ENTRY_DECLARE(.rodata, os_task, os_task_cfg_t, task_id)

#define DeclareExtendedTask(section, task_id)                                                                           \
	OS_LINK_ENTRY_DECLARE(section, os_obj, os_task_inst_t, task_id);                                                 \
	OS_LINK_ENTRY_DECLARE(section, os_obj_context, os_ctx_inst_t, task_id);                                       \
	OS_LINK_ENTRY_DECLARE(.rodata, os_task, os_task_cfg_t, task_id)

#define TASKCFGVAR(task_id) OS_LINK_ENTRY_NAME(os_task, task_id)
#define TASKOBJVAR(task_id) OS_LINK_ENTRY_NAME(os_obj, task_id)

#define OS_START_SEC_CODE
#include "os_memmap.h"

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
extern FUNC(Os_StatusType, OS_CODE) get_task_app_id(ApplicationIDType *ApplicationIDRef);
#endif

#define OS_STOP_SEC_CODE
#include "os_memmap.h"

/*! Task stack reference */
#define TASKSTACKVAR(task_id) (&(OS_LINK_ENTRY_NAME(OsStack, task_id)[0]))

/*! Task stack size */
#define TASKSTACKSIZEVAR(byte_size) STACKSIZE_ROUND_UP(byte_size, StackType_t)
#ifdef __cplusplus
}
#endif

#endif /*  AUTOSAROS_INC_OS_TASK_H */
