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

#ifndef RT_FRAMEWORK_INC_OS_TASK_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_TASK_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_task_types.h"
#include "rtfw_list.h"
#include "rtfw_context_internal.h"
#include "rtfw_core_types.h"
#ifdef __NuttX__
#include <setjmp.h>
#include <nuttx/semaphore.h>
#include <nuttx/event.h>
#include <nuttx/sched.h>
#endif

/* Global Macro Definition */

#define OS_TASK_NO_ACTIVATION_CNT ((uint8)0) /* 任务没有激活 */
#define OS_TASK_MIN_ACTIVATION_CNT ((uint8)1) /* 任务最小激活次数 */
#if (CONFIG_RT_FRAMEWORK_TASK_MULTIPLE_ACTIVATION == 1)
#define OS_TASK_MAX_ACTIVATION_CNT ((uint8)255) /* 任务最大激活次数 */
#else
#define OS_TASK_MAX_ACTIVATION_CNT ((uint8)1) /* 任务最大激活次数 */
#endif /* CONFIG_RT_FRAMEWORK_TASK_MULTIPLE_ACTIVATION */

/* 通过 os_task_cfg_t(task_cfg) 获取各种 object ID */
#define OS_TASK_GET_OWNER_APPID(task_cfg) (OS_APPLICATION_GET_APPID(OS_TASK_GET_APPONRWECFG_REF(task_cfg)))
#define OS_TASK_GET_OWNER_COREID(task_cfg) (OS_CORE_GET_COREID(OS_TASK_GET_CORE_REF(task_cfg)))

/* 通过 os_task_cfg_t(task_cfg) 获取 (task_cfg) 的内部信息 */
#define OS_TASK_GET_CONTEXT_REF(task_cfg) (&((task_cfg)->context))
#define OS_TASK_GET_BASEPRIORITY(task_cfg) ((task_cfg)->base_priority)
#define OS_TASK_GET_TASKID(task_cfg) ((task_cfg)->task_id)
#define OS_TASK_GET_AUTOSTARTMODE(task_cfg) ((task_cfg)->autostart_mode)
#define OS_TASK_GET_ACTIVATIONLIMIT(task_cfg) ((task_cfg)->activation_limit)
#define OS_TASK_GET_ISEXTENDED(task_cfg) ((task_cfg)->is_extended)

/* 通过 os_task_cfg_t(task_cfg) 获取 os_ctx_cfg_t(ContextRef) 的内部信息 */
#define OS_TASK_GET_INSTANCE_REF(task_cfg) ((os_task_inst_t *)((task_cfg)->context.instance))
#define OS_TASK_GET_OBJNAME(task_cfg) ((task_cfg)->context.instance_name)
#define OS_TASK_GET_CONTEXTINST_REF(task_cfg) (&(OS_TASK_GET_INSTANCE_REF(task_cfg)->ctx_instance))
#define OS_TASK_GET_CORE_REF(task_cfg) ((task_cfg)->context.core)
#define OS_TASK_GET_INTAPISTATE_REF(task_cfg) ((task_cfg)->context.int_api_state)
#define OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg) (&((task_cfg)->context.arch_ctx_cfg))
#define OS_TASK_GET_APPONRWECFG_REF(task_cfg) ((task_cfg)->context.app_owner_cfg)
#define GET_APP_CFG_BY_TASK_ID(task_id) ((os_task_cfg_ref_table[task_id])->context.app_owner_cfg)
#define GET_APP_ID_BY_TASK_ID(task_id) ((os_task_cfg_ref_table[task_id])->context.app_owner_cfg->app_id)
#define OS_TASK_GET_ACCESSINGAPPLMASK(task_cfg) ((task_cfg)->context.access_app_mask)
#define OS_TASK_GET_INITCONTEXTID(task_cfg) ((task_cfg)->context.initial_ctx_id)
#define OS_TASK_GET_USEFPU(task_cfg) ((task_cfg)->context.use_fpu)

/* 通过 os_task_cfg_t(task_cfg) 获取 ContextInstType(ctx_instance) 的内部信息 */
#define OS_TASK_GET_CURRENT_CONTEXTID(task_cfg) ((OS_TASK_GET_CONTEXTINST_REF(task_cfg))->current_ctx_id)

#define OS_TASK_GET_LOCKLIST_REF(task_cfg) (&((OS_TASK_GET_CONTEXTINST_REF(task_cfg))->lock_list))

/* 通过 os_task_cfg_t(task_cfg)  获取 os_arch_ctx_cfg_t(arch_ctx_cfg) 的内部信息 */
#define OS_TASK_GET_STACKADDR(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->stack_addr)
#define OS_TASK_GET_STACKSIZE(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->stack_size)
#define OS_TASK_GET_ENTRYADDR(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->entry_addr)
#define OS_TASK_GET_EXITADDR(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->exit_addr)
#define OS_TASK_GET_ENTRYPARAM(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->entry_param)

#define OS_TASK_GET_TASKREF_BY_ID(task_id) (os_task_cfg_ref_table[task_id])

/* Global Enumeration Definition */

/* Global Type & Structure Definition */
struct os_task_inst_s {
	os_ctx_inst_t ctx_instance; /*! Context instance. Must be the first member !!! */
	uint32 activate_cnt;
	jmp_buf jmpbuf;
	struct tcb_s tcb;
	sem_t act_sem;
	nxevent_wait_t wait;
	nxevent_t event;
}; /* os_task_inst_t */

/*! Task configuration by user */
struct os_task_cfg_s {
	/* 32 bit */
	os_ctx_cfg_t context; /*! Context configuration. Must be the first member !!! */

	OsTaskPrioType base_priority; /*! The configured priority of a task. */
	/* enum */

	/* 16 bit */
	TaskType task_id; /*! Task identifier */

	/* 8 bit */
	AppModeType autostart_mode; /*! Application modes where the task shall be activated during OS initialization. */
	uint8 activation_limit; /*! Task activation limited, which must be > 1 */
	boolean is_extended; /*! Tells whether a task is extended (TRUE) or not (FALSE). */
}; /* os_task_cfg_t */

DeclareRtfwRefTable_H(task) /*! Reference to OS task configuration table  */

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

extern FUNC(void, OS_CODE) os_task_init(const os_task_cfg_t *const os_task_cfg_table[], OsObjIndexType size);
extern FUNC(void, OS_CODE) os_disable_level_km(void);
extern FUNC(void, OS_CODE) os_enable_level_km(void);
extern FUNC(Os_StatusType, OS_CODE) os_activate_task(TaskType task_id);
extern FUNC(Os_StatusType, OS_CODE) os_activate_task_asyn(TaskType task_id);
extern FUNC(Os_StatusType, OS_CODE) os_terminate_task(void);
extern FUNC(Os_StatusType, OS_CODE) os_chain_task(TaskType task_id);
extern FUNC(Os_StatusType, OS_CODE) os_get_task_id(TaskRefType task_id_ref);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_TASK_INTERNAL_H */
