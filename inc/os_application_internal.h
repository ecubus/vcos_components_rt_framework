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

#ifndef AUTOSAROS_INC_OS_APPLICATION_INTERNAL_H
#define AUTOSAROS_INC_OS_APPLICATION_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include "Os_Types.h"
#include "os_task_types.h"
#include "os_isr_types.h"
#include "os_application_types.h"
#include "os_counter_types.h"
#include "Compiler.h"
/* coverity[misra_c_2012_rule_20_9_violation:SUPPRESS] */
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "os_schedtable.h"
#endif

/* Global Macro Definition */
#define OS_APPLICATION_GET_APPID(ApplicationRef) ((ApplicationRef)->app_id)
#define OS_APPLICATION_GET_APP_OWNERCOREID(ApplicationRef) ((ApplicationRef)->core_id)
#define INVALID_OSAPPLICATION (os_app_cfg_ref_table_size)
/* Global Enumeration Definition */

/* Global Type & Structure Definition */

/*! Application Instance Description */
struct os_app_inst {
	ApplicationStateType state; /*! Application State. */
};

/*! Constant application configuration information. */
struct os_app_cfg_s {
	os_app_inst_t *instance; /*! Application Instance Description */
	const char *app_name; /*! App name */

	const os_task_cfg_t *const *task_refs; /*! Tasks. */
	const OsObjIndexType *task_count; /*! Number of tasks. */

	const os_isr_cfg_t *const *isr_refs; /*! ISRs of category 2. */
	const OsObjIndexType *isr_count; /*! Number of ISRs of category 2. */
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	const os_schedtable_cfg_t *const *scht_refs; /*! List of Schedule Tables of this core. */
	const OsObjIndexType *scht_count; /*! Number of Schedule Tables. */
#endif
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
	const os_counter_cfg_t *const *counter_refs;
	const OsObjIndexType *counter_count;
#endif
	CoreIdType core_id; /*! The core to which this application belongs. */
	ApplicationIDType app_id; /*! Application ID. */
	boolean IsPrivileged; /*! Tells whether the application is privileged. */
};

/* Global Variable Definition */
DeclareOsRefTable_H(app)

/* Global Function Declaration */

#define OS_START_SEC_CODE
#include "os_memmap.h"
extern FUNC(Os_StatusType, OS_CODE) os_get_app_state(ApplicationType app_id, ApplicationStateRefType state);
extern FUNC(ApplicationType, OS_CODE) os_get_app_id(void);
extern FUNC(ApplicationType, OS_CODE) os_get_current_app_id(void);
extern FUNC(Os_StatusType, OS_CODE) os_allow_access(void);
extern FUNC(ApplicationType, OS_CODE) os_check_object_ownership(ObjectTypeType object_type, Os_ObjectIdType object_id);
extern FUNC(void, OS_CODE) os_app_init(const os_app_cfg_t *const os_app_cfg_table[], OsObjIndexType size);

#define OS_STOP_SEC_CODE
#include "os_memmap.h"

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
static inline Os_StatusType os_app_check_access(ApplicationIDType app_id, uint32 AccessMask)
{
	uint32 AccessMaskTemp = 0;

	if (app_id > (ApplicationIDType)31) {
		/* unsupported for current vcos version */
		AccessMaskTemp = 0U;
	} else {
		AccessMaskTemp = AccessMask;
	}
	/* coverity[misra_c_2012_rule_12_2_violation:SUPPRESS] */
	return (((AccessMaskTemp & (OS_APPID2MASK(app_id))) > 0U) ? OS_STATUS_OK : OS_STATUS_ACCESSRIGHTS_1);
}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

static inline Os_StatusType os_app_check_is_accessible(const os_app_cfg_t *App)
{
	ApplicationStateType appState;
	Os_StatusType status = OS_STATUS_OK;

	status = os_get_app_state(App->app_id, &appState);
	if (status == OS_STATUS_OK) {
		return ((appState == APPLICATION_ACCESSIBLE) ? OS_STATUS_OK : OS_STATUS_NOT_OK);
	}
	return OS_STATUS_NOT_OK;
}

/* Global Function Implementation */

#ifdef __cplusplus
}
#endif

#endif /* AUTOSAROS_INC_OS_APPLICATION_INTERNAL_H */
