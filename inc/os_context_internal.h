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

#ifndef OS_CONTEXT_INTERNAL_H
#define OS_CONTEXT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif
#include <arch/irq.h>
#include "Compiler.h"
#include "os_context_types.h"
#include "os_interrupt_internal.h"
#include "os_interrupt_cbk.h"
#include "os_list.h"
#include "Os_Types.h"
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
#include "os_application_internal.h"
#endif

/* Global Macro Definition */

#define OS_MAX_HOOKNESTINGDEPTH (8U)
#define OS_CONFIG_MAX_INTERRUPT_NESTING_DEPTH (1U)
#define OS_CONTEXT_NESTING_MIN_DEPTH ((os_context_cnt_t)0)
#define OS_CONTEXT_ALIVE ((os_context_state_t)0)
#define OS_CONTEXT_KILLED ((os_context_state_t)1)

#define CONTEXTOBJVAR(context_id) OS_LINK_ENTRY_NAME(os_obj_context, context_id)

#define TASKCONTEXTOBJVAR(context_id) CONTEXTOBJVAR(context_id)

#define HOOKCONTEXTOBJVAR(context_id) CONTEXTOBJVAR(context_id)

#define ISRCONTEXTOBJVAR(core, isr) OS_LINK_ENTRY_NAME(os_obj_context, core##isr)

#define OS_CONTEXT_NESTING_MAX_DEPTH                                                                                   \
	((os_context_cnt_t)(OS_CONFIG_MAX_INTERRUPT_NESTING_DEPTH + OS_MAX_HOOKNESTINGDEPTH))

#define OS_CONTEXT_GET_TASK_CFG_REF(ctx_ref) ((os_task_cfg_t *)(ctx_ref))

#define OS_CONTEXT_GET_ISR_CFG_REF(ctx_ref) ((os_isr_cfg_t *)(ctx_ref))

#define OS_CONTEXT_GET_HOOK_CFG_REF(ctx_ref) ((OsHookCfgType *)(ctx_ref))

#define OS_CONTEXT_GET_TASK_INSTANCE_REF(ctx_ref) ((os_task_inst_t *)(OS_CONTEXT_GET_INSTANCE_REF(ctx_ref)))

#define OS_CONTEXT_GET_ISR_INSTANCE_REF(ctx_ref) ((os_isr_inst_t *)(OS_CONTEXT_GET_INSTANCE_REF(ctx_ref)))

#define OS_CONTEXT_GET_HOOK_INSTANCE_REF(ctx_ref) ((OsHookInstType *)(OS_CONTEXT_GET_INSTANCE_REF(ctx_ref)))

#define OS_CONTEXT_GET_OWNER_APPID(ctx_ref) (OS_APPLICATION_GET_APPID((ctx_ref)->app_owner_cfg))

#define OS_CONTEXT_GET_CURRENT_APPID(ctx_ref) (OS_APPLICATION_GET_APPID((ctx_ref)->ctx_instance->current_app_cfg))

#define OS_CONTEXT_GET_OWNER_COREID(ctx_ref) (OS_CORE_GET_COREID((os_core_cfg_t *)((ctx_ref)->core)))

#define OS_CONTEXT_GET_CURRENT_TASKID(ctx_ref) (OS_TASK_GET_TASKID(OS_CONTEXT_GET_TASK_CFG_REF(ctx_ref)))

#define OS_CONTEXT_GET_CURRENT_ISRID(ctx_ref) (OS_ISR_GET_ISRID(OS_CONTEXT_GET_ISR_CFG_REF(ctx_ref)))

#define OS_CONTEXT_GET_CURRENT_HOOKID(ctx_ref) (OS_HOOK_GET_HOOKID(OS_CONTEXT_GET_HOOK_CFG_REF(ctx_ref)))

#define OS_CONTEXT_GET_INSTANCE_REF(ctx_ref) ((ctx_ref)->instance)

#define OS_CONTEXT_GET_OBJNAME(ctx_ref) ((ctx_ref)->instance_name)

#define OS_CONTEXT_GET_CONTEXTINST_REF(ctx_ref) ((ctx_ref)->ctx_instance)

#define OS_CONTEXT_GET_CORE_REF(ctx_ref) ((os_core_cfg_t *)((ctx_ref)->core))

#define OS_CONTEXT_GET_INTAPISTATE_REF(ctx_ref) ((ctx_ref)->int_api_state)

#define OS_CONTEXT_GET_ARCHCONTEXTCFG_REF(ctx_ref) (&((ctx_ref)->arch_ctx_cfg))

#define OS_CONTEXT_GET_APPONRWECFG_REF(ctx_ref) ((ctx_ref)->app_owner_cfg)

#define OS_CONTEXT_GET_CURRENTAPPCFG_REF(ctx_ref) ((ctx_ref)->ctx_instance->current_app_cfg)

#define OS_CONTEXT_GET_ACCESSINGAPPLMASK(ctx_ref) ((ctx_ref)->access_app_mask)

#define OS_CONTEXT_GET_INITCONTEXTID(ctx_ref) ((ctx_ref)->initial_ctx_id)

#define OS_CONTEXT_GET_USEFPU(ctx_ref) ((ctx_ref)->use_fpu)

#define OS_CONTEXT_GET_CURRENT_CONTEXTID(ctx_ref) ((OS_CONTEXT_GET_CONTEXTINST_REF(ctx_ref))->current_ctx_id)

#define OS_CONTEXT_GET_CURRENT_CONTEXTSTATE(ctx_ref) ((OS_CONTEXT_GET_CONTEXTINST_REF(ctx_ref))->ctx_state)

#define OS_CONTEXT_GET_LOCKLIST_REF(ctx_ref) (&((OS_CONTEXT_GET_CONTEXTINST_REF(ctx_ref))->lock_list))

#define OS_CONTEXTINST_GET_CURRENT_CONTEXTID(ContextInstRef) ((ContextInstRef)->current_ctx_id)

#define OS_CONTEXTINST_GET_CURRENT_CONTEXTSTATE(ContextInstRef) ((ContextInstRef)->ctx_state)

#define OS_CONTEXTINST_GET_LOCKLIST_REF(ContextInstRef) (&((ContextInstRef)->lock_list))

#define OS_ARCHCONTEXT_GET_STACKADDR(arch_ctx_ref) ((arch_ctx_ref)->stack_addr)

#define OS_ARCHCONTEXT_GET_STACKSIZE(arch_ctx_ref) ((arch_ctx_ref)->stack_size)

#define OS_ARCHCONTEXT_GET_ENTRYADDR(arch_ctx_ref) ((arch_ctx_ref)->entry_addr)

#define OS_ARCHCONTEXT_GET_EXITADDR(arch_ctx_ref) ((arch_ctx_ref)->exit_addr)

#define OS_ARCHCONTEXT_GET_ENTRYPARAM(arch_ctx_ref) ((arch_ctx_ref)->entry_param)

#define OS_CONTEXTNEST_GET_CONTEXTREF(os_ctx_next_ref) ((os_ctx_next_ref)->ctx_ref)

#define OS_CONTEXTNEST_GET_COUNT(os_ctx_next_ref) ((os_ctx_next_ref)->count)

#define OS_ARCHISR_GET_LEVEL(arch_isr_ref) ((arch_isr_ref)->level)

#define OS_ARCHISR_GET_VECTORNUM(arch_isr_ref) ((arch_isr_ref)->vector_num)

#define OS_ARCHISR_GET_HWHANDLER(arch_isr_ref) ((arch_isr_ref)->hw_handler)

/* Global Enumeration Definition */

typedef sint16 os_context_cnt_t;
typedef void *os_context_entry_para_t;
typedef void (*os_context_entry_t)(void *);
typedef void (*os_context_exit_t)(void *);
typedef uint8 os_context_state_t;
struct os_context_nest {
	const os_ctx_cfg_t *ctx_ref[OS_CONTEXT_NESTING_MAX_DEPTH]; /*! Lists of currently interrupted threads. */

	os_context_cnt_t count; /*! Counter for interrupted contexts. */
};

/* 上下文的配置信息 */
struct os_arch_context_cfg {
	OsStackAddrType stack_addr; /*! The stack address of this context . */

	OsStackSizeType stack_size; /*! The stack size of this context (in Bytes). */

	os_context_entry_t entry_addr; /*! The execution start address of this context. */

	os_context_exit_t exit_addr; /*! The Exit address of this context. */

	os_context_entry_para_t entry_param; /*! Parameters of Entry */

}; /* os_arch_ctx_cfg_t */

struct os_arch_isr_cfg {
	int_level_t level; /*! ISR优先级 */

	int_vector_num_t vector_num; /*! 硬件中断号 */

	int_vector_num_t irq; /*! 中断注册号（level 或者 vector_num） */

	void *hw_handler;
}; /* os_arch_isr_cfg_t */

struct os_context_inst {
	os_context_id_t current_ctx_id; /*! Current contextID. */

	os_list_t lock_list; /*! task 获取到的 resource 和 spinlock, 用于对 Lock 的 LIFO 进行校验 */

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
	const os_app_cfg_t *current_app_cfg;
#endif /* CONFIG_RT_FRAMEWORK_MULTIAPP */

	os_context_state_t ctx_state;
}; /* os_ctx_inst_t */

/*! context configuration by user */
struct os_context_cfg {
	void *instance; /* Os Object Instance Handler, like os_task_inst, os_isr_inst, OsHookInst, 使用时做指针强转 */

	const char *const instance_name; /*! Object name, like task name, isr name, hook name */

	os_ctx_inst_t *ctx_instance; /*! context Instance */

	const void *core; /*! The core to which this process belongs. */

	os_int_api_state_t *int_api_state; /*! Current state of the interrupt API. */

	os_arch_ctx_cfg_t arch_ctx_cfg; /*! The context configuration info which is specified to HW. */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
	os_app_cfg_t *app_owner_cfg; /*! Application that owns this task */
	AccessApplMaskType access_app_mask; /*! Applications that may access when APPLICATION_ACCESSIBLE */
#endif /* CONFIG_RT_FRAMEWORK_MULTIAPP */

	os_context_id_t initial_ctx_id; /*! Initial contextID. */

#if defined(CONFIG_RT_FRAMEWORK_FPU_CONTEXT) && (CONFIG_RT_FRAMEWORK_FPU_CONTEXT == 1)
	uint32 use_fpu; /*! Use FPU or not. */
#endif /* CONFIG_RT_FRAMEWORK_FPU_CONTEXT */

}; /* os_ctx_cfg_t */

/* Global Variable Declaration */

/* Global Function Declaration */

/* 设置 context */
static inline void os_context_set_contexthdl(const os_ctx_cfg_t *const ctx_cfg);

/* 获取 context */
static inline os_ctx_cfg_t *os_context_get_contexthdl(void);

/* 主要是初始化 os_context_inst 部分 */
static inline void os_context_init_context_instance(const os_ctx_cfg_t *const ctx_cfg);

/* 初始化 ctx_next  */
static inline void os_context_init_context_nest(os_ctx_nest_t *const ctx_next);

/* ctx_next 增加 */
static inline os_context_cnt_t os_context_nest_inc(os_ctx_nest_t *const ctx_next,
						  const os_ctx_cfg_t *const ctx_ref);

/* ctx_next 减少 */
static inline const os_ctx_cfg_t *os_context_nest_dec(os_ctx_nest_t *const ctx_next,
							 const os_ctx_cfg_t *const ctx_ref);

/* 设置 os_ctx_inst_t 中的 current_ctx_id */
static inline void os_context_set_curcontext_id(const os_ctx_cfg_t *const ctx_ref,
					       os_context_id_t current_ctx_id);
/* 获取 os_ctx_inst_t 中的 current_ctx_id */
static inline os_context_id_t os_context_get_curcontext_id(const os_ctx_cfg_t *const ctx_ref);
/* 切换 current_ctx_id，写入新值，返回旧值 */
static inline os_context_id_t os_context_switch_curcontext_id(const os_ctx_cfg_t *const ctx_ref,
							       os_context_id_t current_ctx_id);

/* 检查 中断是否处于 Disabled 状态 */
static inline boolean os_context_check_irq_disabled(const os_ctx_cfg_t *const ctx_ref);
/* 检查当前上下文类型是否允许访问  TRUE: 允许访问, FALSE: 不允许访问 */
static inline boolean os_context_check_curcontext_id(const os_ctx_cfg_t *const ctx_ref,
						    os_context_id_t context_id);

/* 检查 lock_list 是否为空, TRUE: Empty； FALSE：Not Empty */
static inline boolean os_context_check_locklist_empty(const os_ctx_cfg_t *const ctx_ref);

/* Global Function Implementation */

static inline void os_context_set_contexthdl(const os_ctx_cfg_t *const ctx_cfg)
{
	up_set_contexthdl((void *const)ctx_cfg);
}

static inline os_ctx_cfg_t *os_context_get_contexthdl(void)
{
	/* coverity[misra_c_2012_rule_11_5_violation:SUPPRESS] */
	return up_get_contexthdl();
}

static inline os_context_id_t os_get_curcontext_id(void)
{
	const os_ctx_cfg_t *const current_ctx = os_context_get_contexthdl();

	return OS_CONTEXT_GET_CURRENT_CONTEXTID(current_ctx);
}

static inline void os_set_curcontext_id(os_context_id_t context_id)
{
	const os_ctx_cfg_t *const current_ctx = os_context_get_contexthdl();

	OS_CONTEXT_GET_CURRENT_CONTEXTID(current_ctx) = context_id;
}

/* 主要是初始化 os_context_inst 部分 */
static inline void os_context_init_context_instance(const os_ctx_cfg_t *const ctx_cfg)
{
	os_ctx_inst_t *ctx_instance = ctx_cfg->ctx_instance;

	ctx_instance->current_ctx_id = ctx_cfg->initial_ctx_id;
	ctx_instance->ctx_state = OS_CONTEXT_ALIVE;
	OsListInitEmpty(&(ctx_instance->lock_list));
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
	ctx_instance->current_app_cfg = ctx_cfg->app_owner_cfg;
#endif /* CONFIG_RT_FRAMEWORK_MULTIAPP */
}

static inline void os_context_init_context_nest(os_ctx_nest_t *const ctx_next)
{
	os_context_cnt_t i;

	ctx_next->count = OS_CONTEXT_NESTING_MIN_DEPTH;

	for (i = OS_CONTEXT_NESTING_MIN_DEPTH; i < OS_CONTEXT_NESTING_MAX_DEPTH; i++) {
		ctx_next->ctx_ref[i] = NULL_PTR;
	}
}

/* 返回值：
 * OS_CONTEXT_NESTING_MAX_DEPTH：超过最大嵌套深度
 * Others: 最新嵌套深度
 */
static inline os_context_cnt_t os_context_nest_inc(os_ctx_nest_t *const ctx_next,
						  const os_ctx_cfg_t *const ctx_ref)
{
	os_context_cnt_t CurrCnt;

	if (ctx_next->count < OS_CONTEXT_NESTING_MAX_DEPTH) {
		ctx_next->ctx_ref[ctx_next->count] = ctx_ref;
		ctx_next->count++;
		CurrCnt = ctx_next->count;
	} else {
		CurrCnt = OS_CONTEXT_NESTING_MAX_DEPTH;
	}

	return CurrCnt;
}

/* 返回值：
 * NULL_PTR：非法嵌套
 * Others: 嵌套深度 -1 时丢弃的上下文
 */
static inline const os_ctx_cfg_t *os_context_nest_dec(os_ctx_nest_t *const ctx_next,
							 							const os_ctx_cfg_t *const ctx_ref)
{
	const os_ctx_cfg_t *PreContext;

	if (ctx_next->count > 0) {
		ctx_next->count--;
		PreContext = ctx_next->ctx_ref[ctx_next->count];
		ctx_next->ctx_ref[ctx_next->count] = ctx_ref;
	} else {
		PreContext = NULL_PTR;
		/* coverity[misra_c_2012_rule_14_3_violation:SUPPRESS] */
	}

	return PreContext;
}

/* 设置 os_ctx_inst_t 中的 current_ctx_id */
static inline void os_context_set_curcontext_id(const os_ctx_cfg_t *const ctx_ref,
												os_context_id_t current_ctx_id)
{
	OS_CONTEXT_GET_CURRENT_CONTEXTID(ctx_ref) = current_ctx_id;
}

/* 获取 os_ctx_inst_t 中的 current_ctx_id */
static inline os_context_id_t os_context_get_curcontext_id(const os_ctx_cfg_t *const ctx_ref)
{
	return OS_CONTEXT_GET_CURRENT_CONTEXTID(ctx_ref);
}

/* 切换 current_ctx_id，写入新值，返回旧值 */
static inline os_context_id_t os_context_switch_curcontext_id(const os_ctx_cfg_t *const ctx_ref,
																os_context_id_t current_ctx_id)
{
	os_context_id_t PrvContextID = OS_CONTEXT_GET_CURRENT_CONTEXTID(ctx_ref);

	OS_CONTEXT_GET_CURRENT_CONTEXTID(ctx_ref) = current_ctx_id;
	return PrvContextID;
}

/* 检查 中断是否处于 Disabled 状态 */
static inline boolean os_context_check_irq_disabled(const os_ctx_cfg_t *const ctx_ref)
{
	return os_interrupt_is_in_critical_section(OS_CONTEXT_GET_INTAPISTATE_REF(ctx_ref));
}

/* 获取 os_ctx_inst_t 中的 current_ctx_id
 * TRUE: 允许访问
 * FALSE: 不允许访问
 */
static inline boolean os_context_check_curcontext_id(const os_ctx_cfg_t *const ctx_ref,
													os_context_id_t context_id)
{
	return ((OS_CONTEXT_GET_CURRENT_CONTEXTID(ctx_ref) & context_id) == 0u) ? FALSE : TRUE;
}

/* 检查 lock_list 是否为空, TRUE: Empty； FALSE：Not Empty */
static inline boolean os_context_check_locklist_empty(const os_ctx_cfg_t *const ctx_ref)
{
	/* coverity[misra_c_2012_rule_11_3_violation:SUPPRESS] */
	return OS_LIST_IS_EMPTY(OS_CONTEXT_GET_LOCKLIST_REF(ctx_ref));
}

static inline boolean os_context_is_any_task(const os_ctx_cfg_t *const ctx_ref)
{
	return ((OS_CONTEXT_GET_INITCONTEXTID(ctx_ref) & OS_CONTEXT_ANYTASK)) != FALSE;
}

static inline boolean os_context_is_any_isr(const os_ctx_cfg_t *const ctx_ref)
{
	return ((OS_CONTEXT_GET_INITCONTEXTID(ctx_ref) & OS_CONTEXT_ANYISR)) != FALSE;
}

#ifdef __cplusplus
}
#endif

#endif /* OS_CONTEXT_INTERNAL_H */
