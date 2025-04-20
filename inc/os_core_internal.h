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

#ifndef AUTOSAROS_INC_OS_CORE_INTERNAL_H
#define AUTOSAROS_INC_OS_CORE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include "Os_Types.h"
#include "os_core_types.h"
#include "os_context.h"
#include "Compiler.h"

#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
#include "os_ipicall_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
#include "os_application_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_MULTIAPP */

#include "os_interrupt_internal.h"

/* Global Macro Definition */

/* 获取 os_core_cfg_t 中的信息 */
#define OS_CORE_GET_CORE_INSTANCE_REF(CoreRef) ((CoreRef)->instance)
#define OS_CORE_GET_COREID(CoreRef) ((CoreRef)->core_id)
#define OS_CORE_GET_PHYSICAL_COREID(CoreRef) ((CoreRef)->physical_id)

#define OS_CORE_GET_STARTUP_HOOK(CoreRef) ((CoreRef)->StartupHook)
#define OS_CORE_GET_SHUTDOWN_HOOK(CoreRef) ((CoreRef)->ShutdownHook)
#define OS_CORE_GET_ERROR_HOOK(CoreRef) ((CoreRef)->ErrorHook)
#define OS_CORE_GET_PROTECTION_HOOK(CoreRef) ((CoreRef)->ProtectionHook)

#define OS_CORE_GET_KERNEL_APP(CoreRef) ((CoreRef)->kernel_app)

/* 通过 os_core_cfg_t 获取 OsCoreInstanceType 中的信息 */
#define OS_CORE_GET_INTAPISTATE_REF(CoreRef) (&((OS_CORE_GET_CORE_INSTANCE_REF(CoreRef))->int_api_state))
#define OS_CORE_GET_EXPFRAME_REF(CoreRef) ((OS_CORE_GET_CORE_INSTANCE_REF(CoreRef))->ExptFrame)
#define OS_CORE_GET_CONTEXTNEST_REF(CoreRef) (&((OS_CORE_GET_CORE_INSTANCE_REF(CoreRef))->ctx_next))

/* 获取 OsCoreInstanceType 中的信息 */
#define OS_CORE_INSTANCE_GET_INTAPISTATE_REF(CoreInstRef) (&((CoreInstRef)->int_api_state))
#define OS_CORE_INSTANCE_GET_WXPFRAME_REF(CoreInstRef) ((CoreInstRef)->ExptFrame)
#define OS_CORE_INSTANCE_GET_CONTEXTNEST_REF(CoreInstRef) (&((CoreInstRef)->ctx_next))

/* Global Enumeration Definition */
/* coverity[misra_c_2012_rule_2_3_violation:SUPPRESS] */
typedef enum {
	/*! before StartCore(). */
	OS_CORESTARTSTATE_NO_START = 0,
	/*! After StartCore() before StartOS(). */
	OS_CORESTARTSTATE_START_CORE,
	/*! After StartOS(). */
	OS_CORESTARTSTATE_START_OS,
	/* coverity[misra_c_2012_rule_2_3_violation:SUPPRESS] */
} Os_CoreStartStateType;

/* Global Type & Structure Definition */
struct os_core_inst {
	uint32 status; /*! Core status */
	os_int_api_state_t int_api_state; /*! Core Api Interrupt state */
	os_context_id_t ContextID; /*! Current ContextID*/
#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)
	OsErrorType LastError;
#endif
#if defined(CONFIG_RT_FRAMEWORK_CONTEXT_ENTITY) && (CONFIG_RT_FRAMEWORK_CONTEXT_ENTITY == 1)
	os_ctx_nest_t ctx_next; /*! 上下文嵌套信息 */
#endif /* CONFIG_RT_FRAMEWORK_CONTEXT_ENTITY */
};

/*! Constant core configuration information. */
struct os_core_cfg_s {
	os_core_inst_t *instance; /*! Core Object Description */
	CoreIdType core_id; /*! Logical core ID (tells also whether the core is the master core). */
	CoreIdType physical_id; /*! Physical core ID */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
	const os_app_cfg_t *kernel_app;

	const os_app_cfg_t *const *app_refs; /*! Application reference. */
	const OsObjIndexType *app_count; /*! Number of tasks. */
#endif

#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
	const os_ipicall_cfg_t *const *ipicall_refs; /*! Ipicall reference. */
	const OsObjIndexType *ipicall_count; /*! Number of IpiCalls. */
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
};

DeclareOsRefTable_H(core) /*! Reference to Cores configuration table  */

/* Global Function Declaration */
static inline CoreIdType os_get_core_id(void);
static inline os_int_api_state_t *os_core_get_interrupt_api_stateref(void);

/* Global Function Implementation */

static inline CoreIdType os_get_core_id(void)
{
#ifdef CONFIG_ARCH_HAVE_MULTICPU
	/* coverity[misra_c_2012_rule_10_5_violation:SUPPRESS] */
	return (CoreIdType)up_cpu_index();
#else
	return 0;
#endif
}

static inline os_int_api_state_t *os_core_get_interrupt_api_stateref(void)
{
	os_ctx_cfg_t *const current_ctx = os_context_get_contexthdl();

	return OS_CONTEXT_GET_INTAPISTATE_REF(current_ctx);
}

#ifdef __cplusplus
}
#endif

#endif /* AUTOSAROS_INC_OS_CORE_INTERNAL_H */
