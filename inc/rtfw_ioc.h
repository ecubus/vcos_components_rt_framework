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

#ifndef RT_FRAMEWORK_INC_OS_IOC_H
#define RT_FRAMEWORK_INC_OS_IOC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_ioc_internal.h"

/* Global Macro Definition */
#define DeclareIoc08Send(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_08_send, os_ioc_08_send_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_08_send, const os_ioc_08_send_cfg_t, ioc_id)

#define DeclareIoc08Receive(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_08_receive, os_ioc_08_receive_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_08_receive, const os_ioc_08_receive_cfg_t, ioc_id)

#define DeclareIoc16Send(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_16_send, os_ioc_16_send_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_16_send, const os_ioc_16_send_cfg_t, ioc_id)

#define DeclareIoc16Receive(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_16_receive, os_ioc_16_receive_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_16_receive, const os_ioc_16_receive_cfg_t, ioc_id)

#define DeclareIoc32Send(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_32_send, os_ioc_32_send_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_32_send, const os_ioc_32_send_cfg_t, ioc_id)

#define DeclareIoc32Receive(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_32_receive, os_ioc_32_receive_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_32_receive, const os_ioc_32_receive_cfg_t, ioc_id)

#define DeclareIocRefSend(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_ref_send, os_ioc_ref_send_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_ref_send, const os_ioc_ref_send_cfg_t, ioc_id)

#define DeclareIocRefReceive(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_ref_receive, os_ioc_ref_receive_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_ref_receive, const os_ioc_ref_receive_cfg_t, ioc_id)

#define DeclareIocWrite(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_write, os_ioc_write_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_write, const os_ioc_write_cfg_t, ioc_id)

#define DeclareIocRead(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_read, os_ioc_read_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_read, const os_ioc_read_cfg_t, ioc_id)

#define IOC08SENDCFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_08_send, ioc_id)
#define IOC08RECEIVECFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_08_receive, ioc_id)
#define IOC16SENDCFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_16_send, ioc_id)
#define IOC16RECEIVECFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_16_receive, ioc_id)
#define IOC32SENDCFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_32_send, ioc_id)
#define IOC32RECEIVECFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_32_receive, ioc_id)
#define IOCREFSENDCFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_ref_send, ioc_id)
#define IOCREFRECEIVECFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_ref_receive, ioc_id)
#define IOCWRITECFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_write, ioc_id)
#define IOCREADCFGVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_ioc_read, ioc_id)

#define IOC08SENDOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_08_send, ioc_id)
#define IOC08RECEIVEOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_08_receive, ioc_id)
#define IOC16SENDOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_16_send, ioc_id)
#define IOC16RECEIVEOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_16_receive, ioc_id)
#define IOC32SENDOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_32_send, ioc_id)
#define IOC32RECEIVEOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_32_receive, ioc_id)
#define IOCREFSENDOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_ref_send, ioc_id)
#define IOCREFRECEIVEOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_ref_receive, ioc_id)
#define IOCWRITEOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_write, ioc_id)
#define IOCREADOBJVAR(ioc_id) RTFW_LINK_ENTRY_NAME(os_obj_ioc_read, ioc_id)

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(StatusType, OS_CODE) Ioc08Send(IocIdType ioc_id, uint8 data);
FUNC(StatusType, OS_CODE) Ioc08Receive(IocIdType ioc_id, uint8 *data);
FUNC(StatusType, OS_CODE) Ioc08EmptyQueue(IocIdType ioc_id);

FUNC(StatusType, OS_CODE) Ioc16Send(IocIdType ioc_id, uint16 data);
FUNC(StatusType, OS_CODE) Ioc16Receive(IocIdType ioc_id, uint16 *data);
FUNC(StatusType, OS_CODE) Ioc16EmptyQueue(IocIdType ioc_id);

FUNC(StatusType, OS_CODE) Ioc32Send(IocIdType ioc_id, uint32 data);
FUNC(StatusType, OS_CODE) Ioc32Receive(IocIdType ioc_id, uint32 *data);
FUNC(StatusType, OS_CODE) Ioc32EmptyQueue(IocIdType ioc_id);

FUNC(StatusType, OS_CODE) IocRefSend(IocIdType ioc_id, void const *data);
FUNC(StatusType, OS_CODE) IocRefReceive(IocIdType ioc_id, void *data);
FUNC(StatusType, OS_CODE) IocRefEmptyQueue(IocIdType ioc_id);

FUNC(StatusType, OS_CODE) IocWrite(IocIdType ioc_id, void const *data);
FUNC(StatusType, OS_CODE) IocRead(IocIdType ioc_id, void *data);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /*! RT_FRAMEWORK_INC_OS_IOC_H */
