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
#include <nuttx/config.h>
#include "string.h"
#include "os_counter.h"
#include "os_core.h"
#include "os_task.h"
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#include "Os_Hook.h"
#endif

#include "os_ipicall.h"
#include "os_event.h"

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
#include "os_application.h"
#include "application_cfg.h"
#endif

#define OS_IPICALL_ACTIVATETASK ((os_ipicall_service_id_t)0)
#define OS_IPICALL_ACTIVATETASKASYN ((os_ipicall_service_id_t)1)
#define OS_IPICALL_GETAPPLICATIONSTATE ((os_ipicall_service_id_t)2)
#define OS_IPICALL_GETEVENT ((os_ipicall_service_id_t)3)
#define OS_IPICALL_SETEVENT ((os_ipicall_service_id_t)4)
#define OS_IPICALL_SETEVENTASYN ((os_ipicall_service_id_t)5)
#define OS_IPICALL_START_SCHEDULE_TABLE_REL ((os_ipicall_service_id_t)6)
#define OS_IPICALL_START_SCHEDULE_TABLE_ABS ((os_ipicall_service_id_t)7)
#define OS_IPICALL_STOP_SCHEDULE_TABLE ((os_ipicall_service_id_t)8)
#define OS_IPICALL_GET_SCHEDULE_TABLE_STATUS ((os_ipicall_service_id_t)9)
#define OS_IPICALL_NEXT_SCHEDULE_TABLE ((os_ipicall_service_id_t)10)
#define OS_IPICALL_NEXT_SCHEDULE_TABLE_FROM_STOP ((os_ipicall_service_id_t)11)
#define OS_IPICALL_START_SCHEDULE_TABLE_SYN ((os_ipicall_service_id_t)12)
#define OS_IPICALL_SYNC_SCHEDULE_TABLE ((os_ipicall_service_id_t)13)
#define OS_IPICALL_SET_SCHEDULE_TABLE_ASYNC ((os_ipicall_service_id_t)14)
#define OS_IPICALL_SERVICE_MAX ((os_ipicall_service_id_t)15)

#define OS_START_SEC_CONST
#include "os_memmap.h"

CONST(os_ipicall_service_t, OS_CONST)
os_ipicall_service_table[OS_IPICALL_SERVICE_MAX] = {
	[OS_IPICALL_ACTIVATETASK] = &os_ipicall_recv_activate_task,
	[OS_IPICALL_ACTIVATETASKASYN] = &os_ipicall_recv_activate_task_asyn,
	[OS_IPICALL_GETAPPLICATIONSTATE] = &os_ipicall_recv_get_application_state,
	[OS_IPICALL_GETEVENT] = &os_ipicall_recv_get_event,
	[OS_IPICALL_SETEVENT] = &os_ipicall_recv_set_event,
	[OS_IPICALL_SETEVENTASYN] = &os_ipicall_recv_set_event_asyn,
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	[OS_IPICALL_START_SCHEDULE_TABLE_REL] = &os_ipicall_recv_start_scheduletable_rel,
	[OS_IPICALL_START_SCHEDULE_TABLE_ABS] = &os_ipicall_recv_start_scheduletable_abs,
	[OS_IPICALL_STOP_SCHEDULE_TABLE] = &os_ipicall_recv_stop_scheduletable,
	[OS_IPICALL_GET_SCHEDULE_TABLE_STATUS] = &os_ipicall_recv_get_scheduletable_status,
	[OS_IPICALL_NEXT_SCHEDULE_TABLE] = &os_ipicall_recv_next_scheduletable,
	[OS_IPICALL_NEXT_SCHEDULE_TABLE_FROM_STOP] = &os_ipicall_recv_next_scheduletable_from_stop,
	[OS_IPICALL_START_SCHEDULE_TABLE_SYN] = &os_ipicall_recv_start_scheduletable_synchron,
	[OS_IPICALL_SYNC_SCHEDULE_TABLE] = &os_ipicall_recv_sync_scheduletable,
	[OS_IPICALL_SET_SCHEDULE_TABLE_ASYNC] = &os_ipicall_recv_set_scheduletable_async
#endif
};

#define OS_STOP_SEC_CONST
#include "os_memmap.h"

static inline void increment_one(os_ipicall_index_t *idx)
{
	if (*idx >= (os_ipicall_index_t)(QUEUE_DEPTH - 1)) {
		*idx = 0;
	} else {
		*idx = *idx + 1;
	}
}

#define OS_START_SEC_CODE
#include "os_memmap.h"

static FUNC(void, OS_CODE) os_ipicall_isr_handler(void);

/* Local Function Implementation */

/* global Function Declaration */

/* 核间调用发送接口 */
static inline Os_StatusType os_ipicall_send(uint32 target_core, os_ipicall_data_t *data)
{
	Os_StatusType IpiCallSendStatus;
#if (VCOS_CONFIG_IPICALL_SYNC == 1)
	IpiCallSendStatus = os_ipicall_send_sync(target_core, data);
#else /* VCOS_CONFIG_IPICALL_ASYNC */
	IpiCallSendStatus = os_ipicall_send_async(target_core, data);
#endif /* VCOS_CONFIG_IPICALL_SYNC */

	return IpiCallSendStatus;
}

static inline Os_StatusType os_ipicall_get_send_cfg(uint32 target_core, const os_ipicall_cfg_t **ipicall_cfg)
{
	Os_StatusType status = OS_STATUS_IPICALL_NOT_OK;
	const os_core_cfg_t *core_cfg = OS_CORE_CFG_GET(os_get_core_id());

	if (target_core < (uint32)CONFIG_RT_FRAMEWORK_NCORES) {
		*ipicall_cfg = core_cfg->ipicall_refs[target_core];
		if (*ipicall_cfg == NULL_PTR) {
			status = OS_STATUS_IPICALL_UNAVAILABLE;
		} else {
			status = OS_STATUS_OK;
		}
	} else {
		status = OS_STATUS_IPICALL_UNAVAILABLE;
	}
	return status;
}

static inline Os_StatusType os_ipicall_get_send_buffer(const volatile os_ipicall_cfg_t *ipicall_cfg,
						       os_ipicall_data_t *data)
{
	Os_StatusType status = OS_STATUS_IPICALL_NOT_OK;
	os_ipicall_index_t next_send_idx;

	next_send_idx = ipicall_cfg->instance->send.send_idx;
	increment_one(&next_send_idx);
	while (next_send_idx != ipicall_cfg->instance->send.result_idx) {
		if (ipicall_cfg->instance->send_buffer[next_send_idx].ack == OS_IPICALL_ACK) {
			data->index = ipicall_cfg->instance->send.send_idx;
			status = OS_STATUS_OK;
			break;
		}
		ipicall_cfg->instance->send.send_idx = next_send_idx;
		increment_one(&next_send_idx);
	}

	return status;
}

static inline boolean os_ipicall_new_request_available(const volatile os_ipicall_cfg_t *ipicall_cfg)
{
	return (ipicall_cfg->instance->receive.receive_idx != ipicall_cfg->instance->send.send_idx);
}

static inline boolean os_ipicall_request_handled(const volatile os_ipicall_cfg_t *ipicall_cfg,
						 const os_ipicall_data_t *data)
{
	os_ipicall_inst_t *ipicall_inst = ipicall_cfg->instance;
	os_ipicall_index_t index = data->index;

	if (ipicall_inst->send_buffer[index].ack == OS_IPICALL_SYNC_PENDING) {
		return TRUE;
	}

	if (ipicall_inst->receive.receive_idx == ipicall_inst->send.result_idx) {
		return FALSE;
	}

	if (ipicall_inst->receive.receive_idx > ipicall_inst->send.result_idx) {
		return ((index >= ipicall_inst->send.result_idx) && (index < ipicall_inst->receive.receive_idx));
	} else {
		return ((index >= ipicall_inst->send.result_idx) || (index < ipicall_inst->receive.receive_idx));
	}
}

/* global Function Declaration */
FUNC(void, OS_CODE) os_ipicall_init(const os_ipicall_cfg_t *const OsIpiCallCfgTbl[], OsObjIndexType count)
{
	if ((OsIpiCallCfgTbl == NULL_PTR) || (count == (OsObjIndexType)0U)) {
		return;
	}
	const os_ipicall_cfg_t *ipicall_cfg;
	os_ipicall_inst_t *ipicall_inst;
	OsObjIndexType index, Index1;
	CoreIdType core_id = os_get_core_id();

	for (index = 0; index < count; index++) {
		ipicall_cfg = OsIpiCallCfgTbl[index];
		if (ipicall_cfg == NULL_PTR) {
			continue;
		} else {
			ipicall_inst = ipicall_cfg->instance;
			if (ipicall_inst == NULL_PTR) {
				continue;
			} else {
				if (ipicall_cfg->src_core_id == core_id) {
					ipicall_inst->send.send_idx = 0U;
					ipicall_inst->send.result_idx = 0U;
					for (Index1 = 0; Index1 < QUEUE_DEPTH; Index1++) {
						ipicall_inst->send_buffer[Index1].ack = OS_IPICALL_ACK;
					}
				}
				if (ipicall_cfg->target_core_id == core_id) {
					ipicall_inst->receive.receive_idx = 0U;
				}
			}
		}
	}
	/* attach ipi callback */
	up_ipicall_attach((void *)(uintptr_t)os_ipicall_isr_handler);
}

/* Internal implementation of inter-core call sending interface */
static FUNC(Os_StatusType, OS_CODE) Os_IpiCallSendInternal(const volatile os_ipicall_cfg_t *ipicall_cfg,
							   os_ipicall_data_t *data, os_ipicall_cast_mode broad_cast)
{
	Os_StatusType status = OS_STATUS_IPICALL_NOT_OK;
	os_int_state_t int_state;

	os_int_suspend_kernel(&int_state);
	{
		/* handle the result index */
		while (ipicall_cfg->instance->send.result_idx != ipicall_cfg->instance->receive.receive_idx) {
			/* There is a need to increment the result_idx in the channel. */
			if (ipicall_cfg->instance->send_buffer[ipicall_cfg->instance->send.result_idx].ack ==
			    OS_IPICALL_ASYNC_NACK) {
				ipicall_cfg->instance->send_buffer[ipicall_cfg->instance->send.result_idx].ack =
					OS_IPICALL_ACK;
			} else if (ipicall_cfg->instance->send_buffer[ipicall_cfg->instance->send.result_idx].ack ==
				   OS_IPICALL_SYNC_NACK) {
				ipicall_cfg->instance->send_buffer[ipicall_cfg->instance->send.result_idx].ack =
					OS_IPICALL_SYNC_PENDING;
			} else {
				/* do nothing */
			}
			/* Increment result_idx */
			increment_one(&(ipicall_cfg->instance->send.result_idx));
		}
		/* Obtain an available buffer slot. */
		status = os_ipicall_get_send_buffer(ipicall_cfg, data);
		if (status != OS_STATUS_OK) {
			status = OS_STATUS_IPICALL_OVERFLOW;
		} else {
			ipicall_cfg->instance->send_buffer[data->index].service = data->send_element.service;
			ipicall_cfg->instance->send_buffer[data->index].in_param = data->send_element.in_param;
			ipicall_cfg->instance->send_buffer[data->index].send_app_id = data->send_element.send_app_id;
			ipicall_cfg->instance->send_buffer[data->index].ack = data->send_element.ack;
			/* increment sendidx */
			increment_one(&(ipicall_cfg->instance->send.send_idx));
			/* trigger target core interrupt */
			if (broad_cast == OS_IPICALL_UNICAST) {
				status = (Os_StatusType)up_trigger_ipicall((CoreIdType)(ipicall_cfg->target_core_id));
			}
		}
	}
	os_int_resume_global(&int_state);
	return status;
}

/* synchronous inter-processor function call */
FUNC(Os_StatusType, OS_CODE) os_ipicall_send_sync(uint32 target_core, os_ipicall_data_t *data)
{
	Os_StatusType status = OS_STATUS_IPICALL_NOT_OK;
	const os_ipicall_cfg_t *send_cfg = NULL_PTR;
	ApplicationIDType app_id = 0;

	if (data == NULL_PTR) {
		status = OS_STATUS_IPICALL_PARAM_POINTER_2;
	} else {
		if (OS_STATUS_OK != get_task_app_id(&app_id)) {
			status = OS_STATUS_IPICALL_ID_1;
		} else {
			if (app_id > (ApplicationIDType)OS_APPLICATION_CNT) {
				status = OS_STATUS_IPICALL_ID_1;
			} else {
				data->send_element.send_app_id = app_id;
				data->send_element.ack = OS_IPICALL_SYNC_NACK;

				/* get communication channel from static configs */
				status = os_ipicall_get_send_cfg(target_core, &send_cfg);
				if (status != OS_STATUS_OK) {
					status = OS_STATUS_IPICALL_UNAVAILABLE;
				} else {
					/* send(copy) the info to the target core */
					status = Os_IpiCallSendInternal(send_cfg, data, OS_IPICALL_UNICAST);
					if (status == OS_STATUS_OK) {
						/* Polling the results */
						while (os_ipicall_request_handled(send_cfg, data) != TRUE) {
							/* busy loop. can be preemptive by high priority task */
						}
						/* copy results from sharebuffer to private buffer */
						(void)memcpy((uint8 *)&data->ret_element,
							     &send_cfg->instance->result_buffer[data->index],
							     sizeof(os_ipicall_return_t));
						send_cfg->instance->send_buffer[data->index].ack = OS_IPICALL_ACK;
						status = data->ret_element.return_status;
					} else {
						/*! do nothing */
					}
				}
			}
		}
	}

	return status;
}

/*! broad_cast to non-local cores  */
FUNC(Os_StatusType, OS_CODE) os_ipicall_broad_cast(uint32 target_core_mask, os_ipicall_data_t *data)
{
	Os_StatusType ipicall_status = OS_STATUS_IPICALL_NOT_OK;
	const os_ipicall_cfg_t *send_cfg = NULL_PTR;
	ApplicationIDType app_id = 0;

	if (data == NULL_PTR) {
		ipicall_status = OS_STATUS_IPICALL_PARAM_POINTER_2;
	} else {
		if (OS_STATUS_OK != get_task_app_id(&app_id)) {
			ipicall_status = OS_STATUS_IPICALL_ID_1;
		} else {
			if (app_id > (ApplicationIDType)OS_APPLICATION_CNT) {
				ipicall_status = OS_STATUS_IPICALL_ID_1;
			} else {
				data->send_element.send_app_id = app_id;
				data->send_element.ack = OS_IPICALL_ASYNC_NACK;
				uint32 CoreIdx = 0U;

				for (; CoreIdx < (uint32)(CONFIG_RT_FRAMEWORK_NCORES); ++CoreIdx) {
					if (target_core_mask & ((0x00000001U << CoreIdx))) {
						/* get communication channel from static configs */
						ipicall_status = os_ipicall_get_send_cfg(CoreIdx, &send_cfg);
						if (ipicall_status != OS_STATUS_OK) {
							ipicall_status = OS_STATUS_IPICALL_UNAVAILABLE;
						} else {
							/* send(copy) the info to the target core */
							ipicall_status = Os_IpiCallSendInternal(send_cfg, data,
												OS_IPICALL_BROADCAST);
						}
						if (ipicall_status != OS_STATUS_OK)
							break;
					}
				}
				up_trigger_ipicalls(target_core_mask);
			}
		}
	}
	return ipicall_status;
}

/* asynchronous inter-processor function call */
FUNC(Os_StatusType, OS_CODE) os_ipicall_send_async(uint32 target_core, os_ipicall_data_t *data)
{
	Os_StatusType status = OS_STATUS_IPICALL_NOT_OK;
	const os_ipicall_cfg_t *send_cfg = NULL_PTR;
	ApplicationIDType app_id = 0;

	if (data == NULL_PTR) {
		status = OS_STATUS_IPICALL_PARAM_POINTER_2;
	} else {
		if (OS_STATUS_OK != get_task_app_id(&app_id)) {
			status = OS_STATUS_IPICALL_ID_1;
		} else {
			if (app_id > (uint8)OS_APPLICATION_CNT) {
				status = OS_STATUS_IPICALL_ID_1;
			} else {
				data->send_element.send_app_id = app_id;
				data->send_element.ack = OS_IPICALL_ASYNC_NACK;

				/* get communication channel from static configs */
				status = os_ipicall_get_send_cfg(target_core, &send_cfg);
				if (status != OS_STATUS_OK) {
					status = OS_STATUS_IPICALL_UNAVAILABLE;
				} else {
					/* send(copy) the info to the target core */
					status = Os_IpiCallSendInternal(send_cfg, data, OS_IPICALL_UNICAST);
				}
			}
		}
	}

	return status;
}

/*! ipicall services handler */
static FUNC(void, OS_CODE) os_ipicall_isr_handler(void)
{
	Os_StatusType ipicall_status;
	const os_ipicall_cfg_t *ipicall_cfg;
	os_ipicall_inst_t *ipicall_inst;
	os_ipicall_data_t com_data;
	const os_core_cfg_t *core_cfg = OS_CORE_CFG_GET(os_get_core_id());
	os_ipicall_index_t recv_idx;
	os_int_state_t int_state;
	os_ipicall_service_t ipicall_service;
	os_ipicall_send_t *ipicall_send_ptr;
	os_ipicall_return_t *ipicall_ret_ptr;
	uint32 service_idx;

	os_int_suspend_kernel(&int_state);
	for (service_idx = (uint32)CONFIG_RT_FRAMEWORK_NCORES;
	     service_idx < (((uint32)CONFIG_RT_FRAMEWORK_NCORES) * 2U); service_idx++) {
		ipicall_cfg = core_cfg->ipicall_refs[service_idx];
		if (ipicall_cfg == NULL_PTR) {
			continue;
		}
		ipicall_inst = ipicall_cfg->instance;
		if (ipicall_inst == NULL_PTR) {
			continue;
		}
		if (os_ipicall_new_request_available(ipicall_cfg) == TRUE) {
			do {
				recv_idx = ipicall_inst->receive.receive_idx;
				ipicall_send_ptr = &(ipicall_inst->send_buffer[recv_idx]);
				if (ipicall_send_ptr->ack != OS_IPICALL_ACK) {
					com_data.send_element.service = ipicall_send_ptr->service;
					com_data.send_element.in_param = ipicall_send_ptr->in_param;
					com_data.send_element.send_app_id = ipicall_send_ptr->send_app_id;
					ipicall_service = ipicall_cfg->service_ref[com_data.send_element.service];
					ipicall_ret_ptr = &(ipicall_inst->result_buffer[recv_idx]);

					if ((com_data.send_element.service < OS_IPICALL_SERVICE_MAX) &&
					    (ipicall_service != NULL_PTR)) {
						ipicall_status = ipicall_service(&com_data);
					} else {
						ipicall_status = OS_STATUS_IPICALL_UNAVAILABLE;
					}
					ipicall_ret_ptr->service = com_data.send_element.service;
					ipicall_ret_ptr->out_param = com_data.ret_element.out_param;
					ipicall_ret_ptr->return_status = ipicall_status;
				}
				increment_one(&(ipicall_inst->receive.receive_idx));
			} while (os_ipicall_new_request_available(ipicall_cfg) == TRUE);
		}
	}
	os_int_resume_global(&int_state);
}

/*! activate task belongs to other core */
FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_activate_task(os_ipicall_data_t *param)
{
	return os_activate_task(param->send_element.in_param.avtivate_task_param.task_id);
}

/*! activate task belongs to other core */
FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_activate_task_asyn(os_ipicall_data_t *param)
{
	return os_activate_task_asyn(param->send_element.in_param.avtivate_task_param.task_id);
}

/*! get application state */
FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_get_application_state(os_ipicall_data_t *param)
{
	Os_StatusType Os_IpiCallRecvGetApplicationStatus;
	ApplicationStateType AppState = APPLICATION_TERMINATED;

	Os_IpiCallRecvGetApplicationStatus =
		os_get_app_state(param->send_element.in_param.get_app_state_param.app_id, &AppState);
	param->ret_element.out_param.get_app_state_param.AppState = AppState;
	return Os_IpiCallRecvGetApplicationStatus;
}

/*! get event values */
FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_get_event(os_ipicall_data_t *param)
{
	Os_StatusType status;
	os_event_mask_t mask = 0;

	status = os_get_event(param->send_element.in_param.get_event_param.task_id, &mask);
	param->ret_element.out_param.get_event_param.mask = mask;
	return status;
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_set_event(os_ipicall_data_t *param)
{
	return os_set_event(param->send_element.in_param.set_event_param.task_id,
			    param->send_element.in_param.set_event_param.mask);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_set_event_asyn(os_ipicall_data_t *param)
{
	return os_set_event_asyn(param->send_element.in_param.set_event_param.task_id,
				 param->send_element.in_param.set_event_param.mask);
}

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_start_scheduletable_rel(os_ipicall_data_t *param)
{
	return os_start_schedule_table_rel(param->send_element.in_param.start_schtbl_rel_param.schedtable_id,
					   param->send_element.in_param.start_schtbl_rel_param.offset);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_start_scheduletable_abs(os_ipicall_data_t *param)
{
	return os_start_schedule_table_abs(param->send_element.in_param.start_schtbl_abs_param.schedtable_id,
					   param->send_element.in_param.start_schtbl_abs_param.start);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_stop_scheduletable(os_ipicall_data_t *param)
{
	return os_stop_schedule_table(param->send_element.in_param.stop_schtbl_param.schedtable_id);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_get_scheduletable_status(os_ipicall_data_t *param)
{
	ScheduleTableStatusType SchedTableStatus;
	Os_StatusType RecvGetScheduleTableStatus = os_get_schedule_table_status(
		param->send_element.in_param.get_schtbl_status_param.schedtable_id, &SchedTableStatus);
	param->ret_element.out_param.get_schtbl_status_param.SchedTableStatus = SchedTableStatus;
	(void)SchedTableStatus;
	return RecvGetScheduleTableStatus;
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_next_scheduletable(os_ipicall_data_t *param)
{
	return os_next_schedule_table(param->send_element.in_param.next_schtbl_param.curr_schedtable_id,
				      param->send_element.in_param.next_schtbl_param.next_schedtable_id);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_next_scheduletable_from_stop(os_ipicall_data_t *param)
{
	return os_next_schedule_table_from_stop(
		param->send_element.in_param.next_schtbl_fromstop_param.curr_schedtable_id,
		param->send_element.in_param.next_schtbl_fromstop_param.next_schedtable_id);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_start_scheduletable_synchron(os_ipicall_data_t *param)
{
	return os_start_schedule_table_synchron(param->send_element.in_param.start_schtbl_synchron_param.schedtable_id);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_sync_scheduletable(os_ipicall_data_t *param)
{
	return os_sync_schedule_table(param->send_element.in_param.sync_schtbl_param.schedtable_id,
				      param->send_element.in_param.sync_schtbl_param.tick_value);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_recv_set_scheduletable_async(os_ipicall_data_t *param)
{
	return os_set_schedule_table_async(param->send_element.in_param.set_schtbl_async_param.schedtable_id);
}

#endif

/*! ActivateTask */
FUNC(Os_StatusType, OS_CODE) os_ipicall_activate_task(uint32 target_core, TaskType task_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_ACTIVATETASK;
	param.send_element.in_param.avtivate_task_param.task_id = task_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

/*! ActivateTaskAsync */
FUNC(Os_StatusType, OS_CODE) os_ipicall_activate_task_asyn(uint32 target_core, TaskType task_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_ACTIVATETASKASYN;
	param.send_element.in_param.avtivate_task_param.task_id = task_id;

	return os_ipicall_send_async(target_core, &param);
}

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
FUNC(Os_StatusType, OS_CODE)
os_ipicall_start_schedule_table_rel(uint32 target_core, ScheduleTableType schedtable_id, TickType offset)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_START_SCHEDULE_TABLE_REL;
	param.send_element.in_param.start_schtbl_rel_param.schedtable_id = schedtable_id;
	param.send_element.in_param.start_schtbl_rel_param.offset = offset;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(Os_StatusType, OS_CODE)
os_ipicall_start_schedule_table_abs(uint32 target_core, ScheduleTableType schedtable_id, TickType start)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_START_SCHEDULE_TABLE_ABS;
	param.send_element.in_param.start_schtbl_abs_param.schedtable_id = schedtable_id;
	param.send_element.in_param.start_schtbl_abs_param.start = start;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_stop_schedule_table(uint32 target_core, ScheduleTableType schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_STOP_SCHEDULE_TABLE;
	param.send_element.in_param.stop_schtbl_param.schedtable_id = schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(Os_StatusType, OS_CODE)
os_ipicall_get_schedule_table_status(uint32 target_core, ScheduleTableType schedtable_id,
				     ScheduleTableStatusType *SchedTableStatus)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_GET_SCHEDULE_TABLE_STATUS;
	param.send_element.in_param.get_schtbl_status_param.schedtable_id = schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;
	Os_StatusType GetScheduleTableStatusStatus = os_ipicall_send_sync(target_core, &param);
	*SchedTableStatus = param.ret_element.out_param.get_schtbl_status_param.SchedTableStatus;

	return GetScheduleTableStatusStatus;
}

FUNC(Os_StatusType, OS_CODE)
os_ipicall_next_schedule_table(uint32 target_core, ScheduleTableType curr_schedtable_id,
			       ScheduleTableType next_schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_NEXT_SCHEDULE_TABLE;
	param.send_element.in_param.next_schtbl_param.curr_schedtable_id = curr_schedtable_id;
	param.send_element.in_param.next_schtbl_param.next_schedtable_id = next_schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(Os_StatusType, OS_CODE)
os_ipicall_next_schedule_table_from_stop(uint32 target_core, ScheduleTableType curr_schedtable_id,
					 ScheduleTableType next_schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_NEXT_SCHEDULE_TABLE_FROM_STOP;
	param.send_element.in_param.next_schtbl_fromstop_param.curr_schedtable_id = curr_schedtable_id;
	param.send_element.in_param.next_schtbl_fromstop_param.next_schedtable_id = next_schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(Os_StatusType, OS_CODE)
os_ipicall_start_schedule_table_synchron(uint32 target_core, ScheduleTableType schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_START_SCHEDULE_TABLE_SYN;
	param.send_element.in_param.start_schtbl_synchron_param.schedtable_id = schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send_sync(target_core, &param);
}

FUNC(Os_StatusType, OS_CODE)
os_ipicall_sync_schedule_table(uint32 target_core, ScheduleTableType schedtable_id, TickType tick_value)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SYNC_SCHEDULE_TABLE;
	param.send_element.in_param.sync_schtbl_param.schedtable_id = schedtable_id;
	param.send_element.in_param.sync_schtbl_param.tick_value = tick_value;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send_sync(target_core, &param);
}

FUNC(Os_StatusType, OS_CODE) os_ipicall_set_schedule_table_async(uint32 target_core, ScheduleTableType schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SET_SCHEDULE_TABLE_ASYNC;
	param.send_element.in_param.set_schtbl_async_param.schedtable_id = schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send_sync(target_core, &param);
}

#endif

/*! GetApplicationState*/
FUNC(Os_StatusType, OS_CODE)
os_ipicall_get_app_state(uint32 target_core, ApplicationType app_id, ApplicationStateRefType state)
{
	Os_StatusType status;
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_GETAPPLICATIONSTATE;
	param.send_element.in_param.get_app_state_param.app_id = app_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;
	status = os_ipicall_send_sync(target_core, &param);
	*state = (ApplicationStateType)param.ret_element.out_param.get_app_state_param.AppState;

	return status;
}

/*! GetEvent */
FUNC(Os_StatusType, OS_CODE) os_ipicall_get_event(uint32 target_core, TaskType task_id, os_event_mask_t *mask)
{
	Os_StatusType status;
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_GETEVENT;
	param.send_element.in_param.get_event_param.task_id = task_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;
	status = os_ipicall_send_sync(target_core, &param);
	*mask = param.ret_element.out_param.get_event_param.mask;

	return status;
}

/*! SetEvent */
FUNC(Os_StatusType, OS_CODE) os_ipicall_set_event(uint32 target_core, TaskType task_id, os_event_mask_t mask)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SETEVENT;
	param.send_element.in_param.set_event_param.task_id = task_id;
	param.send_element.in_param.set_event_param.mask = mask;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

/*! SetEventAsync */
Os_StatusType os_ipicall_set_event_asyn(uint32 target_core, TaskType task_id, os_event_mask_t mask)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SETEVENTASYN;
	param.send_element.in_param.set_event_param.task_id = task_id;
	param.send_element.in_param.set_event_param.mask = mask;

	return os_ipicall_send_async(target_core, &param);
}

#define OS_STOP_SEC_CODE
#include "os_memmap.h"
