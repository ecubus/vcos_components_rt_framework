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

#ifndef AUTOSAROS_INC_CORE_H
#define AUTOSAROS_INC_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "Os_Types.h"
#include "os_core_internal.h"

/* Global Macro Definition */

/* Get Core Configuration Information Handler according ID */
#define OS_CORE_CFG_GET(_id) (os_core_cfg_ref_table[_id])

/* Global Enumeration Definition */

/* Global Type & Structure Definition */

/*! Core Instance Description */
/*! Core declare*/
#define DeclareCore(section, core_id)                                                                                   \
	OS_LINK_ENTRY_DECLARE(section, os_obj, os_core_inst_t, core_id);                                                 \
	OS_LINK_ENTRY_DECLARE(.rodata, os_core, os_core_cfg_t, core_id)

#define OSCORECFGVAR(core_id) OS_LINK_ENTRY_NAME(os_core, core_id)

/* coverity[misra_c_2012_rule_20_9_violation:SUPPRESS] */
#define OSCOREOBJVAR(core_id) OS_LINK_ENTRY_NAME(os_obj, core_id)

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "os_memmap.h"

/*!  \brief         启动时第一次核间同步
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */

FUNC(void, OS_CODE) os_core_sync_phase_one(void);

/*!  \brief         启动时第二次核间同步
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) os_core_sync_phase_two(void);

/*!  \brief         下电时核间同步
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) Os_CoreShutdownSync(void);

/*!  \brief         After this service the OS on all AUTOSAR cores is shut down. Allowed at TASK level and ISR level
 *                  and also internally by the OS. The function will never return. The function will force other cores
 *                  into a shutdown.
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        ANY|TASK|ISR1|ISR2|KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) Os_ShutdownAllCores(StatusType Error);

/*!  \brief         Shut down the specific core.
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        ANY|TASK|ISR1|ISR2|KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) Os_ShutdownCore(StatusType Error);

#define OS_STOP_SEC_CODE
#include "os_memmap.h"

/* Global Function Implementation */

#ifdef __cplusplus
}
#endif

#endif /* AUTOSAROS_INC_CORE_H */
/*! @} */
/*! @} */
