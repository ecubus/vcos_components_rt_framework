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
#include "rtfw_ioc.h"
#include "rtfw_error.h"
#include "rtfw_task.h"

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#include "rtfw_hook.h"
#endif

#if (defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)) || \
	(defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1))
#include "rtfw_spinlock_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE || CONFIG_RT_FRAMEWORK_SPINLOCK */

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
#include "rtfw_application.h"
#include "application_cfg.h"
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
static inline
void os_ioc_lock(const os_ioc_cfg_t * const ioc)
{
	switch(ioc->lock_mode)
	{
		case OS_IOCLOCKMODE_NONE:
			break;
		case OS_IOCLOCKMODE_INTERRUPTS:
			os_suspend_os_interrupts();
			break;
#if (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
		case OS_IOCLOCKMODE_SPINLOCKS:
			(void)os_get_raw_spinlock(ioc->spin_id);
			break;
		case OS_IOCLOCKMODE_ALL:
			os_suspend_os_interrupts();
			(void)os_get_raw_spinlock(ioc->spin_id);
			break;
#endif /*CONFIG_RT_FRAMEWORK_SPINLOCK == 1*/
		default:
			break;
	}
}

static inline
void os_ioc_unlock(const os_ioc_cfg_t * const ioc)
{
	switch (ioc->lock_mode)
	{
		case OS_IOCLOCKMODE_NONE:
			break;
		case OS_IOCLOCKMODE_INTERRUPTS:
			os_resume_os_interrupts();
			break;
#if (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
		case OS_IOCLOCKMODE_SPINLOCKS:
			(void)os_release_raw_spinlock(ioc->spin_id);
			break;
		case OS_IOCLOCKMODE_ALL:
			(void)os_release_raw_spinlock(ioc->spin_id);
			os_resume_os_interrupts();
			break;
#endif /*CONFIG_RT_FRAMEWORK_SPINLOCK == 1*/
		default:
			REPORT_FATAL_ERROR();
			break;
	}
}

static inline
uint8 os_ioc_channel_is_enabled(const os_ioc_cfg_t * const ioc)
{
	const os_ioc_inst_t *p_ioc_inst = ioc->obj_desc;

	return ((p_ioc_inst->state) == OS_IOCCHANNELSTATE_ENABLED) ? 1U : 0U;
}

static inline
void os_ioc_close_channel(const os_ioc_cfg_t * const ioc)
{
	os_ioc_inst_t *p_ioc_inst = ioc->obj_desc;

	p_ioc_inst->state =  OS_IOCCHANNELSTATE_DISABLED;
}

static inline
boolean os_fifo_indices_in_range (os_fifo_idx_type write_idx, os_fifo_idx_type read_idx, os_fifo_idx_type size)
{
	return ((write_idx < size) && (read_idx < size));
}

static inline
Os_StatusType os_ioc_call_callbacks (const os_ioc_callbacks_config_t * const callbacks)
{
	Os_StatusType result = OS_STATUS_OK;

	if (NULL != callbacks) {
#if (CONFIG_RT_FRAMEWORK_HOOK == 1)
		for (uint32 callback_index = 0; callback_index < callbacks->callback_count; callback_index++) {
			if (E_OK != os_hook_call_ioc_callback(callbacks->callback_refs[callback_index])) {
				result = OS_STATUS_NOT_OK;
			}
		}
#else
		result = OS_STATUS_NOT_OK;
#endif
	} else {
		result = OS_STATUS_OK;
	}

	return result;
}

static inline
void os_fifo_idx_increment (os_fifo_idx_type *index, os_fifo_idx_type size)
{
	if((*index) >= size) {
		REPORT_FATAL_ERROR();
	}

	if ((*index) == (size -1u))	{
		(*index) = 0;
	} else {
		(*index)++;
	}
}

static inline
boolean os_fifo_is_full(os_fifo_idx_type write_idx, os_fifo_idx_type read_idx, os_fifo_idx_type size)
{
	os_fifo_idx_type write_idx_temp = write_idx;

	os_fifo_idx_increment(&write_idx_temp, size);
	return (write_idx_temp == read_idx);
}

static inline
boolean os_fifo_is_empty (os_fifo_idx_type write_idx, os_fifo_idx_type read_idx)
{
	return (read_idx == write_idx);
}

static inline
void os_ioc_increment_lost_counter(os_ioc_lost_counter_type * const lost_counter)
{
	(*lost_counter)++;
}

static inline
Os_StatusType os_ioc_08_recv_check_no_data_lost(const os_ioc_08_receive_cfg_t  * const p_ioc_recv_cfg)
{
	Os_StatusType result;
	os_ioc_lost_counter_type lost_counter;
	os_ioc_08_receive_inst_t *p_ioc_08_recv_inst = (os_ioc_08_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_08_send_inst_t *p_ioc_08_send_inst = (os_ioc_08_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	lost_counter = p_ioc_08_send_inst->lost_counter;

	if (lost_counter == p_ioc_08_recv_inst->lost_counter_snapshot) {
		result = (Os_StatusType) OS_STATUS_OK;
	} else {
		p_ioc_08_recv_inst->lost_counter_snapshot = lost_counter;

		result = OS_STATUS_IOC_CHECK_FAILED;
	}

	return result;
}

static inline
Os_StatusType os_ioc_16_recv_check_no_data_lost(const os_ioc_16_receive_cfg_t  * const p_ioc_recv_cfg)
{
	Os_StatusType result;
	os_ioc_lost_counter_type lost_counter;
	os_ioc_16_receive_inst_t *p_ioc_16_recv_inst = (os_ioc_16_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_16_send_inst_t *p_ioc_16_send_inst = (os_ioc_16_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	lost_counter = p_ioc_16_send_inst->lost_counter;

	if (lost_counter == p_ioc_16_recv_inst->lost_counter_snapshot) {
		result = (Os_StatusType) OS_STATUS_OK;
	} else {
		p_ioc_16_recv_inst->lost_counter_snapshot = lost_counter;

		result = OS_STATUS_IOC_CHECK_FAILED;
	}

	return result;
}

static inline
Os_StatusType os_ioc_32_recv_check_no_data_lost(const os_ioc_32_receive_cfg_t * const p_ioc_recv_cfg)
{
	Os_StatusType result;
	os_ioc_lost_counter_type lost_counter;
	os_ioc_32_receive_inst_t *p_ioc_32_recv_inst = (os_ioc_32_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_32_send_inst_t *p_ioc_32_send_inst = (os_ioc_32_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	lost_counter = p_ioc_32_send_inst->lost_counter;

	if (lost_counter == p_ioc_32_recv_inst->lost_counter_snapshot) {
		result = (Os_StatusType) OS_STATUS_OK;
	} else {
		p_ioc_32_recv_inst->lost_counter_snapshot = lost_counter;

		result = OS_STATUS_IOC_CHECK_FAILED;
	}

	return result;
}

static inline
Os_StatusType os_ioc_ref_recv_check_no_data_lost(const os_ioc_ref_receive_cfg_t * const p_ioc_recv_cfg)
{
	Os_StatusType result;
	os_ioc_lost_counter_type lost_counter;
	os_ioc_ref_receive_inst_t *p_ioc_ref_recv_inst = (os_ioc_ref_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_ref_send_inst_t *p_ioc_ref_send_inst = (os_ioc_ref_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	lost_counter = p_ioc_ref_send_inst->lost_counter;

	if (lost_counter == p_ioc_ref_recv_inst->lost_counter_snapshot) {
		result = (Os_StatusType) OS_STATUS_OK;
	} else {
		p_ioc_ref_recv_inst->lost_counter_snapshot = lost_counter;

		result = OS_STATUS_IOC_CHECK_FAILED;
	}

	return result;
}

static inline
os_fifo_write_result_type os_fifo_08_enqueue(const os_ioc_08_send_cfg_t * const p_ioc_send_cfg, uint8 val)
{
	os_fifo_write_result_type os_fifo08_enqueue_result;

	os_fifo08_enqueue_result = OS_FIFOWRITERESULT_BUFFEROVERFLOW;
	os_ioc_08_send_inst_t *p_ioc_08_send_inst = (os_ioc_08_send_inst_t *)(p_ioc_send_cfg->ioc.obj_desc);
	const os_ioc_08_receive_inst_t *p_ioc_08_recv_inst =
		(os_ioc_08_receive_inst_t *)
		(p_ioc_send_cfg->receive->ioc.obj_desc);

	if (!os_fifo_is_full(p_ioc_08_send_inst->write_idx, p_ioc_08_recv_inst->read_idx, p_ioc_send_cfg->size)) {
		p_ioc_send_cfg->data[p_ioc_08_send_inst->write_idx] = val;
		os_fifo_idx_increment(&(p_ioc_08_send_inst->write_idx), p_ioc_send_cfg->size);
		os_fifo08_enqueue_result = OS_FIFOWRITERESULT_OK;
	}

	return os_fifo08_enqueue_result;
}

static inline
os_fifo_write_result_type os_fifo_16_enqueue(const os_ioc_16_send_cfg_t * const p_ioc_send_cfg, uint16 val)
{
	os_fifo_write_result_type os_fifo16_enqueue_result;

	os_fifo16_enqueue_result = OS_FIFOWRITERESULT_BUFFEROVERFLOW;
	os_ioc_16_send_inst_t *p_ioc_16_send_inst = (os_ioc_16_send_inst_t *)(p_ioc_send_cfg->ioc.obj_desc);
	const os_ioc_16_receive_inst_t *p_ioc_16_recv_inst =
		(os_ioc_16_receive_inst_t *)
		(p_ioc_send_cfg->receive->ioc.obj_desc);

	if (!os_fifo_is_full(p_ioc_16_send_inst->write_idx, p_ioc_16_recv_inst->read_idx, p_ioc_send_cfg->size)) {
		p_ioc_send_cfg->data[p_ioc_16_send_inst->write_idx] = val;
		os_fifo_idx_increment(&(p_ioc_16_send_inst->write_idx), p_ioc_send_cfg->size);
		os_fifo16_enqueue_result = OS_FIFOWRITERESULT_OK;
	}

	return os_fifo16_enqueue_result;
}

static inline
os_fifo_write_result_type os_fifo_32_enqueue(const os_ioc_32_send_cfg_t * const p_ioc_send_cfg, uint32 val)
{
	os_fifo_write_result_type os_fifo32_enqueue_result;

	os_fifo32_enqueue_result = OS_FIFOWRITERESULT_BUFFEROVERFLOW;
	os_ioc_32_send_inst_t *p_ioc_32_send_inst = (os_ioc_32_send_inst_t *)(p_ioc_send_cfg->ioc.obj_desc);
	const os_ioc_32_receive_inst_t *p_ioc_32_recv_inst =
		(os_ioc_32_receive_inst_t *)
		(p_ioc_send_cfg->receive->ioc.obj_desc);

	if (!os_fifo_is_full(p_ioc_32_send_inst->write_idx, p_ioc_32_recv_inst->read_idx, p_ioc_send_cfg->size)) {
		p_ioc_send_cfg->data[p_ioc_32_send_inst->write_idx] = val;
		os_fifo_idx_increment(&(p_ioc_32_send_inst->write_idx), p_ioc_send_cfg->size);
		os_fifo32_enqueue_result = OS_FIFOWRITERESULT_OK;
	}

	return os_fifo32_enqueue_result;
}

static inline
os_fifo_write_result_type os_fifo_ref_enqueue(const os_ioc_ref_send_cfg_t * const p_ioc_send_cfg, void const *val)
{
	os_fifo_write_result_type os_fifo_ref_enqueue_result = OS_FIFOWRITERESULT_BUFFEROVERFLOW;
	os_ioc_ref_send_inst_t *p_ioc_ref_send_inst = (os_ioc_ref_send_inst_t *)(p_ioc_send_cfg->ioc.obj_desc);
	const os_ioc_ref_receive_inst_t *p_ioc_ref_recv_inst =
		(os_ioc_ref_receive_inst_t *)
		(p_ioc_send_cfg->receive->ioc.obj_desc);

	if (!os_fifo_is_full(p_ioc_ref_send_inst->write_idx, p_ioc_ref_recv_inst->read_idx, p_ioc_send_cfg->size)) {
		p_ioc_send_cfg->write_cbk(p_ioc_send_cfg->data, p_ioc_ref_send_inst->write_idx, val);
		os_fifo_idx_increment(&(p_ioc_ref_send_inst->write_idx), p_ioc_send_cfg->size);
		os_fifo_ref_enqueue_result = OS_FIFOWRITERESULT_OK;
	}

	return os_fifo_ref_enqueue_result;
}

static inline
uint8 os_fifo_08_dequeue(const os_ioc_08_receive_cfg_t  * const p_ioc_recv_cfg)
{
	uint8 os_fifo08_dequeue_result;
	os_ioc_08_receive_inst_t *p_ioc_08_recv_inst = (os_ioc_08_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_08_send_inst_t *p_ioc_08_send_inst = (os_ioc_08_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	if(os_fifo_is_empty(p_ioc_08_send_inst->write_idx, p_ioc_08_recv_inst->read_idx)) {
		REPORT_FATAL_ERROR();
	}
	os_fifo08_dequeue_result = (p_ioc_recv_cfg->data)[p_ioc_08_recv_inst->read_idx];
	os_fifo_idx_increment(&(p_ioc_08_recv_inst->read_idx), p_ioc_recv_cfg->size);

	return os_fifo08_dequeue_result;
}

static inline
uint16 os_fifo_16_dequeue(const os_ioc_16_receive_cfg_t  * const p_ioc_recv_cfg)
{
	uint16 os_fifo16_dequeue_result;
	os_ioc_16_receive_inst_t *p_ioc_16_recv_inst = (os_ioc_16_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_16_send_inst_t *p_ioc_16_send_inst = (os_ioc_16_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	if(os_fifo_is_empty(p_ioc_16_send_inst->write_idx, p_ioc_16_recv_inst->read_idx)) {
		REPORT_FATAL_ERROR();
	}
	os_fifo16_dequeue_result = (p_ioc_recv_cfg->data)[p_ioc_16_recv_inst->read_idx];
	os_fifo_idx_increment(&(p_ioc_16_recv_inst->read_idx), p_ioc_recv_cfg->size);

	return os_fifo16_dequeue_result;
}

static inline
uint32 os_fifo_32_dequeue(const os_ioc_32_receive_cfg_t  * const p_ioc_recv_cfg)
{
	uint32 os_fifo32_dequeue_result;
	os_ioc_32_receive_inst_t *p_ioc_32_recv_inst = (os_ioc_32_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_32_send_inst_t *p_ioc_32_send_inst = (os_ioc_32_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	if(os_fifo_is_empty(p_ioc_32_send_inst->write_idx, p_ioc_32_recv_inst->read_idx)) {
		REPORT_FATAL_ERROR();
	}
	os_fifo32_dequeue_result = (p_ioc_recv_cfg->data)[p_ioc_32_recv_inst->read_idx];
	os_fifo_idx_increment(&(p_ioc_32_recv_inst->read_idx), p_ioc_recv_cfg->size);

	return os_fifo32_dequeue_result;
}

static inline
void os_fifo_ref_dequeue(const os_ioc_ref_receive_cfg_t  * const p_ioc_recv_cfg, void *data_ref)
{
	os_ioc_ref_receive_inst_t *p_ioc_ref_recv_inst = (os_ioc_ref_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_ref_send_inst_t *p_ioc_ref_send_inst = (os_ioc_ref_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	if(os_fifo_is_empty(p_ioc_ref_send_inst->write_idx, p_ioc_ref_recv_inst->read_idx)) {
		REPORT_FATAL_ERROR();
	}

	p_ioc_recv_cfg->read_cbk(p_ioc_recv_cfg->data, p_ioc_ref_recv_inst->read_idx, data_ref);

	os_fifo_idx_increment(&(p_ioc_ref_recv_inst->read_idx), p_ioc_recv_cfg->size);
}

static inline
void os_fifo_empty_queue (os_fifo_idx_type const *write_idx, os_fifo_idx_type *read_idx)
{
	(*read_idx) = (*write_idx);
}

static FUNC(void, OS_CODE)
os_ioc_08_receive_init(const os_ioc_08_receive_cfg_t *p_os_ioc_08_receive_cfg)
{
	os_ioc_08_receive_inst_t *p_os_ioc_08_receive_inst;

	if((NULL != p_os_ioc_08_receive_cfg) && (NULL != p_os_ioc_08_receive_cfg->ioc.obj_desc)) {
		p_os_ioc_08_receive_inst = (os_ioc_08_receive_inst_t *)(p_os_ioc_08_receive_cfg->ioc.obj_desc);
		p_os_ioc_08_receive_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_08_receive_inst->read_idx = 0;
		p_os_ioc_08_receive_inst->lost_counter_snapshot = 0;
	}
}

static FUNC(void, OS_CODE)
os_ioc_08_send_init(const os_ioc_08_send_cfg_t *p_os_ioc08_send_cfg)
{
	os_ioc_08_send_inst_t *p_os_ioc_08_send_inst;

	if ((NULL != p_os_ioc08_send_cfg) && (NULL != p_os_ioc08_send_cfg->ioc.obj_desc)) {
		p_os_ioc_08_send_inst = (os_ioc_08_send_inst_t *)(p_os_ioc08_send_cfg->ioc.obj_desc);
		p_os_ioc_08_send_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_08_send_inst->write_idx = 0;
		p_os_ioc_08_send_inst->lost_counter = 0;
	}
}

static FUNC(void, OS_CODE)
os_ioc_16_receive_init(const os_ioc_16_receive_cfg_t *p_os_ioc_16_receive_cfg)
{
	os_ioc_16_receive_inst_t *p_os_ioc_16_receive_inst;

	if ((NULL != p_os_ioc_16_receive_cfg) && (NULL != p_os_ioc_16_receive_cfg->ioc.obj_desc)) {
		p_os_ioc_16_receive_inst = ( os_ioc_16_receive_inst_t *)(p_os_ioc_16_receive_cfg->ioc.obj_desc);
		p_os_ioc_16_receive_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_16_receive_inst->read_idx = 0;
		p_os_ioc_16_receive_inst->lost_counter_snapshot = 0;
	}
}

static FUNC(void, OS_CODE)
os_ioc_16_send_init(const os_ioc_16_send_cfg_t *p_os_ioc16_send_cfg)
{
	os_ioc_16_send_inst_t *p_os_ioc_16_send_inst;

	if ((NULL != p_os_ioc16_send_cfg) && (NULL != p_os_ioc16_send_cfg->ioc.obj_desc)) {
		p_os_ioc_16_send_inst = (os_ioc_16_send_inst_t *)(p_os_ioc16_send_cfg->ioc.obj_desc);
		p_os_ioc_16_send_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_16_send_inst->write_idx = 0;
		p_os_ioc_16_send_inst->lost_counter = 0;
	}
}

static FUNC(void, OS_CODE)
os_ioc_32_receive_init(const os_ioc_32_receive_cfg_t *p_os_ioc_32_receive_cfg)
{
	os_ioc_32_receive_inst_t *p_os_ioc_32_receive_inst;

	if ((NULL != p_os_ioc_32_receive_cfg) && (NULL != p_os_ioc_32_receive_cfg->ioc.obj_desc)) {
		p_os_ioc_32_receive_inst = (os_ioc_32_receive_inst_t *)(p_os_ioc_32_receive_cfg->ioc.obj_desc);
		p_os_ioc_32_receive_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_32_receive_inst->read_idx = 0;
		p_os_ioc_32_receive_inst->lost_counter_snapshot = 0;
	}
}

static FUNC(void, OS_CODE)
os_ioc_32_send_init(const os_ioc_32_send_cfg_t *p_os_ioc32_send_cfg)
{
	os_ioc_32_send_inst_t *p_os_ioc_32_send_inst;

	if ((NULL != p_os_ioc32_send_cfg) && (NULL != p_os_ioc32_send_cfg->ioc.obj_desc)) {
		p_os_ioc_32_send_inst = (os_ioc_32_send_inst_t *)(p_os_ioc32_send_cfg->ioc.obj_desc);
		p_os_ioc_32_send_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_32_send_inst->write_idx = 0;
		p_os_ioc_32_send_inst->lost_counter = 0;
	}
}

static FUNC(void, OS_CODE)
os_ioc_ref_receive_init(const os_ioc_ref_receive_cfg_t *p_os_ioc_ref_receive_cfg)
{
	os_ioc_ref_receive_inst_t *p_os_ioc_ref_receive_inst;

	if ((NULL != p_os_ioc_ref_receive_cfg) && (NULL != p_os_ioc_ref_receive_cfg->ioc.obj_desc)) {
		p_os_ioc_ref_receive_inst = (os_ioc_ref_receive_inst_t *)(p_os_ioc_ref_receive_cfg->ioc.obj_desc);
		p_os_ioc_ref_receive_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_ref_receive_inst->read_idx = 0;
		p_os_ioc_ref_receive_inst->lost_counter_snapshot = 0;
	}
}

static FUNC(void, OS_CODE)
os_ioc_ref_send_init(const os_ioc_ref_send_cfg_t *p_os_ioc_ref_send_cfg)
{
	os_ioc_ref_send_inst_t *p_os_ioc_ref_send_inst;

	if ((NULL != p_os_ioc_ref_send_cfg) && (NULL != p_os_ioc_ref_send_cfg->ioc.obj_desc)) {
		p_os_ioc_ref_send_inst = (os_ioc_ref_send_inst_t *)(p_os_ioc_ref_send_cfg->ioc.obj_desc);
		p_os_ioc_ref_send_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_ref_send_inst->write_idx = 0;
		p_os_ioc_ref_send_inst->lost_counter = 0;
	}
}

static FUNC(void, OS_CODE)
os_ioc_read_init(const os_ioc_read_cfg_t *p_os_ioc_read_cfg)
{
	os_ioc_read_inst_t *p_os_ioc_read_inst;

	if ((NULL != p_os_ioc_read_cfg) && (NULL != p_os_ioc_read_cfg->ioc.obj_desc)) {
		p_os_ioc_read_inst = (os_ioc_read_inst_t *)(p_os_ioc_read_cfg->ioc.obj_desc);
		p_os_ioc_read_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
	}
}

static FUNC(void, OS_CODE)
os_ioc_write_init(const os_ioc_write_cfg_t *p_os_ioc_write_cfg)
{
	os_ioc_write_inst_t *p_os_ioc_write_inst;

	if ((NULL != p_os_ioc_write_cfg) && (NULL != p_os_ioc_write_cfg->ioc.obj_desc)) {
		p_os_ioc_write_inst = (os_ioc_write_inst_t *)(p_os_ioc_write_cfg->ioc.obj_desc);
		p_os_ioc_write_inst->base.state = OS_IOCCHANNELSTATE_ENABLED;
		p_os_ioc_write_cfg->init_cbk(p_os_ioc_write_cfg->data);       /*TODO: Enter critical user section ??*/
	}
}

FUNC(void, OS_CODE) os_ioc_init(const os_ioc_cfg_t *const os_ioc_cfg_table[], OsObjIndexType size)
{
	if (size == (OsObjIndexType)0U) {
		return;
	}

	const os_ioc_cfg_t *p_ioc_cfg;
	uint16 index = 0;

	for (index = 0; index < size; index++) {
		p_ioc_cfg = os_ioc_cfg_table[index];
		if (p_ioc_cfg == NULL_PTR) {
			continue;
		}
		switch (p_ioc_cfg->sub_class)
		{
			case ioc_08_receive_class:
				os_ioc_08_receive_init((os_ioc_08_receive_cfg_t *)p_ioc_cfg);
				break;
			case ioc_08_send_class:
				os_ioc_08_send_init((os_ioc_08_send_cfg_t *)p_ioc_cfg);
				break;
			case ioc_16_receive_class:
				os_ioc_16_receive_init((os_ioc_16_receive_cfg_t *)p_ioc_cfg);
				break;
			case ioc_16_send_class:
				os_ioc_16_send_init((os_ioc_16_send_cfg_t *)p_ioc_cfg);
				break;
			case ioc_32_receive_class:
				os_ioc_32_receive_init((os_ioc_32_receive_cfg_t *)p_ioc_cfg);
				break;
			case ioc_32_send_class:
				os_ioc_32_send_init((os_ioc_32_send_cfg_t *)p_ioc_cfg);
				break;
			case ioc_ref_receive_class:
				os_ioc_ref_receive_init((os_ioc_ref_receive_cfg_t *)p_ioc_cfg);
				break;
			case ioc_ref_send_class:
				os_ioc_ref_send_init((os_ioc_ref_send_cfg_t *)p_ioc_cfg);
				break;
			case ioc_read_class:
				os_ioc_read_init((os_ioc_read_cfg_t *)p_ioc_cfg);
				break;
			case ioc_write_class:
				os_ioc_write_init((os_ioc_write_cfg_t *)p_ioc_cfg);
				break;
			default:
				break;
		}
	}
}

static FUNC(os_ioc_internal_return_type, OS_CODE)
os_ioc_08_receive_internal (const os_ioc_08_receive_cfg_t * const p_ioc_recv_cfg, uint8 *data)
{
	os_ioc_internal_return_type os_ioc08_receive_status = OS_IOC_E_INTERNAL_OK;
	const os_ioc_08_receive_inst_t *p_ioc_08_recv_inst = (os_ioc_08_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_08_send_inst_t *p_ioc_08_send_inst = (os_ioc_08_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	if (os_ioc_channel_is_enabled(&(p_ioc_recv_cfg->ioc)) == 0u) {
		os_ioc08_receive_status = OS_IOC_E_INTERNAL_CHANNEL_CLOSED;
	} else if (os_ioc_channel_is_enabled(&(p_ioc_recv_cfg->send->ioc)) == 0u) {
		os_ioc08_receive_status = OS_IOC_E_INTERNAL_CLOSE_CHANNEL;
	} else if (!os_fifo_indices_in_range(p_ioc_08_send_inst->write_idx,
										 p_ioc_08_recv_inst->read_idx,
										 p_ioc_recv_cfg->size)) {
		os_ioc08_receive_status = OS_IOC_E_INTERNAL_CLOSE_CHANNEL;
	} else if (os_fifo_is_empty(p_ioc_08_send_inst->write_idx, p_ioc_08_recv_inst->read_idx)) {
		if (os_ioc_08_recv_check_no_data_lost(p_ioc_recv_cfg) == OS_STATUS_IOC_CHECK_FAILED) {
			os_ioc08_receive_status = OS_IOC_E_INTERNAL_LOST_AND_NO_DATA;
		} else {
			os_ioc08_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
		}
	} else {
		if (os_ioc_08_recv_check_no_data_lost(p_ioc_recv_cfg) == OS_STATUS_IOC_CHECK_FAILED) {
			os_ioc08_receive_status = OS_IOC_E_INTERNAL_LOST_DATA;
		} else {
			os_ioc08_receive_status = OS_IOC_E_INTERNAL_OK;
		}

		(*data) = os_fifo_08_dequeue(p_ioc_recv_cfg);
	}

	return os_ioc08_receive_status;
}

static FUNC(os_ioc_internal_return_type, OS_CODE)
os_ioc_16_receive_internal (const os_ioc_16_receive_cfg_t * const p_ioc_recv_cfg, uint16 *data)
{
	os_ioc_internal_return_type os_ioc16_receive_status = OS_IOC_E_INTERNAL_OK;
	const os_ioc_16_receive_inst_t *p_ioc_16_recv_inst = (os_ioc_16_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_16_send_inst_t *p_ioc_16_send_inst = (os_ioc_16_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	if (os_ioc_channel_is_enabled(&(p_ioc_recv_cfg->ioc)) == 0u) {
		os_ioc16_receive_status = OS_IOC_E_INTERNAL_CHANNEL_CLOSED;
	} else if (os_ioc_channel_is_enabled(&(p_ioc_recv_cfg->send->ioc)) == 0u) {
		os_ioc16_receive_status = OS_IOC_E_INTERNAL_CLOSE_CHANNEL;
	} else if (!os_fifo_indices_in_range(p_ioc_16_send_inst->write_idx,
									     p_ioc_16_recv_inst->read_idx,
										 p_ioc_recv_cfg->size))	{
		os_ioc16_receive_status = OS_IOC_E_INTERNAL_CLOSE_CHANNEL;
	} else if (os_fifo_is_empty(p_ioc_16_send_inst->write_idx, p_ioc_16_recv_inst->read_idx)) {
		if (os_ioc_16_recv_check_no_data_lost(p_ioc_recv_cfg) == OS_STATUS_IOC_CHECK_FAILED) {
			os_ioc16_receive_status = OS_IOC_E_INTERNAL_LOST_AND_NO_DATA;
		} else {
			os_ioc16_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
		}
	} else {
		if (os_ioc_16_recv_check_no_data_lost(p_ioc_recv_cfg) == OS_STATUS_IOC_CHECK_FAILED) {
			os_ioc16_receive_status = OS_IOC_E_INTERNAL_LOST_DATA;
		} else {
			os_ioc16_receive_status = OS_IOC_E_INTERNAL_OK;
		}

		(*data) = os_fifo_16_dequeue(p_ioc_recv_cfg);
	}

	return os_ioc16_receive_status;
}

static FUNC(os_ioc_internal_return_type, OS_CODE)
os_ioc_32_receive_internal (const os_ioc_32_receive_cfg_t * const p_ioc_recv_cfg, uint32 *data)
{
	os_ioc_internal_return_type os_ioc32_receive_status = OS_IOC_E_INTERNAL_OK;
	const os_ioc_32_receive_inst_t *p_ioc_32_recv_inst = (os_ioc_32_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_32_send_inst_t *p_ioc_32_send_inst = (os_ioc_32_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	if (os_ioc_channel_is_enabled(&(p_ioc_recv_cfg->ioc)) == 0u) {
		os_ioc32_receive_status = OS_IOC_E_INTERNAL_CHANNEL_CLOSED;
	} else if (os_ioc_channel_is_enabled(&(p_ioc_recv_cfg->send->ioc)) == 0u) {
		os_ioc32_receive_status = OS_IOC_E_INTERNAL_CLOSE_CHANNEL;
	} else if (!os_fifo_indices_in_range(p_ioc_32_send_inst->write_idx,
										 p_ioc_32_recv_inst->read_idx,
										 p_ioc_recv_cfg->size))	{
		os_ioc32_receive_status = OS_IOC_E_INTERNAL_CLOSE_CHANNEL;
	} else if (os_fifo_is_empty(p_ioc_32_send_inst->write_idx, p_ioc_32_recv_inst->read_idx)) {
		if (os_ioc_32_recv_check_no_data_lost(p_ioc_recv_cfg) == OS_STATUS_IOC_CHECK_FAILED) {
			os_ioc32_receive_status = OS_IOC_E_INTERNAL_LOST_AND_NO_DATA;
		} else {
			os_ioc32_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
		}
	} else {
		if (os_ioc_32_recv_check_no_data_lost(p_ioc_recv_cfg) == OS_STATUS_IOC_CHECK_FAILED) {
			os_ioc32_receive_status = OS_IOC_E_INTERNAL_LOST_DATA;
		} else {
			os_ioc32_receive_status = OS_IOC_E_INTERNAL_OK;
		}

		(*data) = os_fifo_32_dequeue(p_ioc_recv_cfg);
	}

	return os_ioc32_receive_status;
}

static FUNC(os_ioc_internal_return_type, OS_CODE)
os_ioc_ref_receive_internal(const os_ioc_ref_receive_cfg_t * const p_ioc_recv_cfg, void *data)
{
	os_ioc_internal_return_type os_ioc_ref_receive_status = OS_IOC_E_INTERNAL_OK;
	const os_ioc_ref_receive_inst_t *p_ioc_ref_recv_inst = (os_ioc_ref_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	const os_ioc_ref_send_inst_t *p_ioc_ref_send_inst = (os_ioc_ref_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);

	if (os_ioc_channel_is_enabled(&(p_ioc_recv_cfg->ioc)) == 0u) {
		os_ioc_ref_receive_status = OS_IOC_E_INTERNAL_CHANNEL_CLOSED;
	} else if (os_ioc_channel_is_enabled(&(p_ioc_recv_cfg->send->ioc)) == 0u) {
		os_ioc_ref_receive_status = OS_IOC_E_INTERNAL_CLOSE_CHANNEL;
	} else if (!os_fifo_indices_in_range(p_ioc_ref_send_inst->write_idx,
										 p_ioc_ref_recv_inst->read_idx,
										 p_ioc_recv_cfg->size ))	{
		os_ioc_ref_receive_status = OS_IOC_E_INTERNAL_CLOSE_CHANNEL;
	} else if (os_fifo_is_empty(p_ioc_ref_send_inst->write_idx, p_ioc_ref_recv_inst->read_idx))	{
		if (os_ioc_ref_recv_check_no_data_lost(p_ioc_recv_cfg) == OS_STATUS_IOC_CHECK_FAILED) {
			os_ioc_ref_receive_status = OS_IOC_E_INTERNAL_LOST_AND_NO_DATA;
		} else {
			os_ioc_ref_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
		}
	} else {
		if (os_ioc_ref_recv_check_no_data_lost(p_ioc_recv_cfg) == OS_STATUS_IOC_CHECK_FAILED) {
			os_ioc_ref_receive_status = OS_IOC_E_INTERNAL_LOST_DATA;
		} else {
			os_ioc_ref_receive_status = OS_IOC_E_INTERNAL_OK;
		}

		os_fifo_ref_dequeue(p_ioc_recv_cfg, data);
	}

	return os_ioc_ref_receive_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_08_send (IocIdType ioc_id, uint8 data)
{
	Os_StatusType ioc08_send_status = OS_STATUS_OK;
	const os_ioc_08_send_cfg_t *p_ioc_send_cfg;
	os_ioc_08_send_inst_t *p_ioc_08_send_inst;
	const os_ioc_08_receive_inst_t *p_ioc_08_recv_inst;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}
	if (TRUE == os_interrupt_is_disabled())	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_send_cfg = (const os_ioc_08_send_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_send_cfg) ||
		 (NULL == p_ioc_send_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_send_cfg->receive) ||
		 (NULL == p_ioc_send_cfg->receive->ioc.obj_desc) ) {
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_send_cfg->ioc.accessing_appl_mask)) ) {
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_fifo_write_result_type os_ioc08_send_result;

	p_ioc_08_send_inst = (os_ioc_08_send_inst_t *)(p_ioc_send_cfg->ioc.obj_desc);
	p_ioc_08_recv_inst = (os_ioc_08_receive_inst_t *)(p_ioc_send_cfg->receive->ioc.obj_desc);
	ioc08_send_status = OS_STATUS_OK;
	os_ioc_lock(&(p_ioc_send_cfg->ioc));

	if (os_ioc_channel_is_enabled(&(p_ioc_send_cfg->ioc)) == 0u) {
		ioc08_send_status = OS_STATUS_LIMIT;
	} else if(os_ioc_channel_is_enabled(&(p_ioc_send_cfg->receive->ioc)) == 0u)	{
		os_ioc_close_channel(&(p_ioc_send_cfg->ioc));
		ioc08_send_status = OS_STATUS_LIMIT;
	} else if (!os_fifo_indices_in_range(p_ioc_08_send_inst->write_idx,
										 p_ioc_08_recv_inst->read_idx,
										 p_ioc_send_cfg->size))	{
		os_ioc_close_channel(&(p_ioc_send_cfg->ioc));
		ioc08_send_status = OS_STATUS_LIMIT;
	} else {
		os_ioc08_send_result = os_fifo_08_enqueue(p_ioc_send_cfg, data);
		if (os_ioc08_send_result == OS_FIFOWRITERESULT_BUFFEROVERFLOW) {
			os_ioc_increment_lost_counter(&(p_ioc_08_send_inst->lost_counter));
			ioc08_send_status = OS_STATUS_IOC_LOST_DATA;
		}
	}

	os_ioc_unlock(&(p_ioc_send_cfg->ioc));
	if (ioc08_send_status == OS_STATUS_OK) {
		ioc08_send_status = os_ioc_call_callbacks(&(p_ioc_send_cfg->callbacks));
	}

	return ioc08_send_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_08_receive (IocIdType ioc_id, uint8 *data)
{
	os_ioc_internal_return_type ioc08_receive_status = OS_IOC_E_INTERNAL_OK;
	const os_ioc_08_receive_cfg_t  *p_ioc_recv_cfg;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	if (NULL_PTR == data) {
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (TRUE == os_interrupt_is_disabled())	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_recv_cfg = (const os_ioc_08_receive_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_recv_cfg) ||
		 (NULL == p_ioc_recv_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_recv_cfg->send) ||
		 (NULL == p_ioc_recv_cfg->send->ioc.obj_desc)) {
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_recv_cfg->ioc.accessing_appl_mask)) ) {
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_ioc_lock(&(p_ioc_recv_cfg->ioc));
	ioc08_receive_status = os_ioc_08_receive_internal(p_ioc_recv_cfg, data);

	if (ioc08_receive_status == OS_IOC_E_INTERNAL_CLOSE_CHANNEL) {
		os_ioc_close_channel(&(p_ioc_recv_cfg->ioc));
		ioc08_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
	} else if (ioc08_receive_status == OS_IOC_E_INTERNAL_CHANNEL_CLOSED) {
		ioc08_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
	} else {
		// Do nothing
	}

	os_ioc_unlock(&(p_ioc_recv_cfg->ioc));
	return (Os_StatusType)ioc08_receive_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_08_empty_queue (IocIdType ioc_id)
{
	Os_StatusType ioc08_empty_queue_status = OS_STATUS_OK;
	const os_ioc_08_receive_cfg_t *p_ioc_recv_cfg;
	os_ioc_08_receive_inst_t *p_ioc_08_recv_inst;
	const os_ioc_08_send_inst_t *p_ioc_08_send_inst;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}
	if (TRUE == os_interrupt_is_disabled())	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_recv_cfg = (const os_ioc_08_receive_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_recv_cfg) ||
		 (NULL == p_ioc_recv_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_recv_cfg->send) ||
		 (NULL == p_ioc_recv_cfg->send->ioc.obj_desc) )	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_recv_cfg->ioc.accessing_appl_mask)) ) {
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	p_ioc_08_recv_inst = (os_ioc_08_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	p_ioc_08_send_inst = (os_ioc_08_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);
	os_fifo_empty_queue(&(p_ioc_08_send_inst->write_idx), &(p_ioc_08_recv_inst->read_idx));
	ioc08_empty_queue_status = OS_STATUS_OK;
	return ioc08_empty_queue_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_16_send (IocIdType ioc_id, uint16 data)
{
	Os_StatusType ioc16_send_status = OS_STATUS_OK;
	const os_ioc_16_send_cfg_t *p_ioc_send_cfg;
	os_ioc_16_send_inst_t *p_ioc_16_send_inst;
	const os_ioc_16_receive_inst_t *p_ioc_16_recv_inst;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}
	if (TRUE == os_interrupt_is_disabled())	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_send_cfg = (const os_ioc_16_send_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_send_cfg) ||
		 (NULL == p_ioc_send_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_send_cfg->receive) ||
		 (NULL == p_ioc_send_cfg->receive->ioc.obj_desc) ) {
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_send_cfg->ioc.accessing_appl_mask)) ) {
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_fifo_write_result_type os_ioc16_send_result;

	p_ioc_16_send_inst = (os_ioc_16_send_inst_t *)(p_ioc_send_cfg->ioc.obj_desc);
	p_ioc_16_recv_inst = (os_ioc_16_receive_inst_t *)(p_ioc_send_cfg->receive->ioc.obj_desc);
	ioc16_send_status = OS_STATUS_OK;
	os_ioc_lock(&(p_ioc_send_cfg->ioc));

	if (os_ioc_channel_is_enabled(&(p_ioc_send_cfg->ioc)) == 0u) {
		ioc16_send_status = OS_STATUS_LIMIT;
	} else if(os_ioc_channel_is_enabled(&(p_ioc_send_cfg->receive->ioc)) == 0u)	{
		os_ioc_close_channel(&(p_ioc_send_cfg->ioc));
		ioc16_send_status = OS_STATUS_LIMIT;
	} else if (!os_fifo_indices_in_range(p_ioc_16_send_inst->write_idx,
									     p_ioc_16_recv_inst->read_idx,
										 p_ioc_send_cfg->size))	{
		os_ioc_close_channel(&(p_ioc_send_cfg->ioc));
		ioc16_send_status = OS_STATUS_LIMIT;
	} else {
		os_ioc16_send_result = os_fifo_16_enqueue(p_ioc_send_cfg, data);
		if (os_ioc16_send_result == OS_FIFOWRITERESULT_BUFFEROVERFLOW) {
			os_ioc_increment_lost_counter(&(p_ioc_16_send_inst->lost_counter));
			ioc16_send_status = OS_STATUS_IOC_LOST_DATA;
		}
	}

	os_ioc_unlock(&(p_ioc_send_cfg->ioc));
	if (ioc16_send_status == OS_STATUS_OK) {
		ioc16_send_status = os_ioc_call_callbacks(&(p_ioc_send_cfg->callbacks));
	}

	return ioc16_send_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_16_receive (IocIdType ioc_id, uint16 *data)
{
	os_ioc_internal_return_type ioc16_receive_status = OS_IOC_E_INTERNAL_OK;
	const os_ioc_16_receive_cfg_t  *p_ioc_recv_cfg;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	if (NULL_PTR == data) {
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (TRUE == os_interrupt_is_disabled())	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_recv_cfg = (const os_ioc_16_receive_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_recv_cfg) ||
		 (NULL == p_ioc_recv_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_recv_cfg->send) ||
		 (NULL == p_ioc_recv_cfg->send->ioc.obj_desc) )	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_recv_cfg->ioc.accessing_appl_mask)) ) {
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_ioc_lock(&(p_ioc_recv_cfg->ioc));
	ioc16_receive_status = os_ioc_16_receive_internal(p_ioc_recv_cfg, data);

	if (ioc16_receive_status == OS_IOC_E_INTERNAL_CLOSE_CHANNEL) {
		os_ioc_close_channel(&(p_ioc_recv_cfg->ioc));
		ioc16_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
	} else if (ioc16_receive_status == OS_IOC_E_INTERNAL_CHANNEL_CLOSED) {
		ioc16_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
	} else {
		// Do nothing
	}

	os_ioc_unlock(&(p_ioc_recv_cfg->ioc));
	return (Os_StatusType)ioc16_receive_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_16_empty_queue (IocIdType ioc_id)
{
	Os_StatusType ioc16_empty_queue_status = OS_STATUS_OK;
	const os_ioc_16_receive_cfg_t *p_ioc_recv_cfg;
	os_ioc_16_receive_inst_t *p_ioc_16_recv_inst;
	const os_ioc_16_send_inst_t *p_ioc_16_send_inst;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}
	if (TRUE == os_interrupt_is_disabled())	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_recv_cfg = (const os_ioc_16_receive_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_recv_cfg) ||
		 (NULL == p_ioc_recv_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_recv_cfg->send) ||
		 (NULL == p_ioc_recv_cfg->send->ioc.obj_desc) )	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_recv_cfg->ioc.accessing_appl_mask)) ) {
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	p_ioc_16_recv_inst = (os_ioc_16_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	p_ioc_16_send_inst = (os_ioc_16_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);
	os_fifo_empty_queue(&(p_ioc_16_send_inst->write_idx), &(p_ioc_16_recv_inst->read_idx));
	ioc16_empty_queue_status = OS_STATUS_OK;
	return ioc16_empty_queue_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_32_send (IocIdType ioc_id, uint32 data)
{
	Os_StatusType ioc32_send_status = OS_STATUS_OK;
	const os_ioc_32_send_cfg_t *p_ioc_send_cfg;
	os_ioc_32_send_inst_t *p_ioc_32_send_inst;
	const os_ioc_32_receive_inst_t *p_ioc_32_recv_inst;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}
	if (TRUE == os_interrupt_is_disabled())	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_send_cfg = (const os_ioc_32_send_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_send_cfg) ||
		 (NULL == p_ioc_send_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_send_cfg->receive) ||
		 (NULL == p_ioc_send_cfg->receive->ioc.obj_desc) ) {
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_send_cfg->ioc.accessing_appl_mask)) ) {
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_fifo_write_result_type os_ioc32_send_result;

	p_ioc_32_send_inst = (os_ioc_32_send_inst_t *)(p_ioc_send_cfg->ioc.obj_desc);
	p_ioc_32_recv_inst = (os_ioc_32_receive_inst_t *)(p_ioc_send_cfg->receive->ioc.obj_desc);
	ioc32_send_status = OS_STATUS_OK;
	os_ioc_lock(&(p_ioc_send_cfg->ioc));

	if (os_ioc_channel_is_enabled(&(p_ioc_send_cfg->ioc)) == 0u) {
		ioc32_send_status = OS_STATUS_LIMIT;
	} else if(os_ioc_channel_is_enabled(&(p_ioc_send_cfg->receive->ioc)) == 0u)	{
		os_ioc_close_channel(&(p_ioc_send_cfg->ioc));
		ioc32_send_status = OS_STATUS_LIMIT;
	} else if (!os_fifo_indices_in_range(p_ioc_32_send_inst->write_idx,
									     p_ioc_32_recv_inst->read_idx,
										 p_ioc_send_cfg->size))	{
		os_ioc_close_channel(&(p_ioc_send_cfg->ioc));
		ioc32_send_status = OS_STATUS_LIMIT;
	} else {
		os_ioc32_send_result = os_fifo_32_enqueue(p_ioc_send_cfg, data);
		if (os_ioc32_send_result == OS_FIFOWRITERESULT_BUFFEROVERFLOW) {
			os_ioc_increment_lost_counter(&(p_ioc_32_send_inst->lost_counter));
			ioc32_send_status = OS_STATUS_IOC_LOST_DATA;
		}
	}

	os_ioc_unlock(&(p_ioc_send_cfg->ioc));
	if (ioc32_send_status == OS_STATUS_OK) {
		ioc32_send_status = os_ioc_call_callbacks(&(p_ioc_send_cfg->callbacks));
	}

	return ioc32_send_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_32_receive (IocIdType ioc_id, uint32 *data)
{
	os_ioc_internal_return_type ioc32_receive_status = OS_IOC_E_INTERNAL_OK;
	const os_ioc_32_receive_cfg_t  *p_ioc_recv_cfg;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	if (NULL_PTR == data) /* 入参为空指针 */
	{
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (TRUE == os_interrupt_is_disabled()) /* 关闭中断后不可调用服务 */
	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_recv_cfg = (const os_ioc_32_receive_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_recv_cfg) ||
		 (NULL == p_ioc_recv_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_recv_cfg->send) ||
		 (NULL == p_ioc_recv_cfg->send->ioc.obj_desc) )
	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_recv_cfg->ioc.accessing_appl_mask)) )
	{
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_ioc_lock(&(p_ioc_recv_cfg->ioc));
	ioc32_receive_status = os_ioc_32_receive_internal(p_ioc_recv_cfg, data);

	if (ioc32_receive_status == OS_IOC_E_INTERNAL_CLOSE_CHANNEL) {
		os_ioc_close_channel(&(p_ioc_recv_cfg->ioc));
		ioc32_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
	} else if (ioc32_receive_status == OS_IOC_E_INTERNAL_CHANNEL_CLOSED) {
		ioc32_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
	} else {
		/* Do nothing */
	}

	os_ioc_unlock(&(p_ioc_recv_cfg->ioc));
	return (Os_StatusType)ioc32_receive_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_32_empty_queue (IocIdType ioc_id)
{
	Os_StatusType ioc32_empty_queue_status = OS_STATUS_OK;
	const os_ioc_32_receive_cfg_t *p_ioc_recv_cfg;
	os_ioc_32_receive_inst_t *p_ioc_32_recv_inst;
	const os_ioc_32_send_inst_t *p_ioc_32_send_inst;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}
	if (TRUE == os_interrupt_is_disabled()) /* 关闭中断后不可调用服务 */
	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_recv_cfg = (const os_ioc_32_receive_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_recv_cfg) ||
		 (NULL == p_ioc_recv_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_recv_cfg->send) ||
		 (NULL == p_ioc_recv_cfg->send->ioc.obj_desc) )
	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_recv_cfg->ioc.accessing_appl_mask)) )
	{
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	p_ioc_32_recv_inst = (os_ioc_32_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	p_ioc_32_send_inst = (os_ioc_32_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);
	os_fifo_empty_queue(&(p_ioc_32_send_inst->write_idx), &(p_ioc_32_recv_inst->read_idx));
	ioc32_empty_queue_status = OS_STATUS_OK;
	return ioc32_empty_queue_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_ref_send (IocIdType ioc_id, void const *data)
{
	Os_StatusType ioc_ref_send_status = OS_STATUS_OK;
	const os_ioc_ref_send_cfg_t *p_ioc_send_cfg;
	os_ioc_ref_send_inst_t *p_ioc_ref_send_inst;
	const os_ioc_ref_receive_inst_t *p_ioc_ref_recv_inst;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	if (NULL_PTR == data) /* 入参为空指针 */
	{
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (TRUE == os_interrupt_is_disabled()) /* 关闭中断后不可调用服务 */
	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_send_cfg = (const os_ioc_ref_send_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_send_cfg) ||
		 (NULL == p_ioc_send_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_send_cfg->receive) ||
		 (NULL == p_ioc_send_cfg->receive->ioc.obj_desc) )
	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_send_cfg->ioc.accessing_appl_mask)) )
	{
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_fifo_write_result_type os_ioc_ref_send_result;

	p_ioc_ref_send_inst = (os_ioc_ref_send_inst_t *)(p_ioc_send_cfg->ioc.obj_desc);
	p_ioc_ref_recv_inst = (os_ioc_ref_receive_inst_t *)(p_ioc_send_cfg->receive->ioc.obj_desc);
	os_ioc_lock(&(p_ioc_send_cfg->ioc));

	if (os_ioc_channel_is_enabled(&(p_ioc_send_cfg->ioc)) == 0u) {
		ioc_ref_send_status = OS_STATUS_LIMIT;
	} else if (os_ioc_channel_is_enabled(&(p_ioc_send_cfg->receive->ioc)) == 0u) {
		os_ioc_close_channel(&(p_ioc_send_cfg->ioc));
		ioc_ref_send_status = OS_STATUS_LIMIT;
	} else if (!os_fifo_indices_in_range(p_ioc_ref_send_inst->write_idx,
										 p_ioc_ref_recv_inst->read_idx,
										 p_ioc_send_cfg->size )) {
		os_ioc_close_channel(&(p_ioc_send_cfg->ioc));
		ioc_ref_send_status = OS_STATUS_LIMIT;
	} else {
		os_ioc_ref_send_result = os_fifo_ref_enqueue(p_ioc_send_cfg, data);

		if (os_ioc_ref_send_result == OS_FIFOWRITERESULT_BUFFEROVERFLOW)
		{
			os_ioc_increment_lost_counter(&(p_ioc_ref_send_inst->lost_counter));
			ioc_ref_send_status = OS_STATUS_IOC_LOST_DATA;
		}
	}

	os_ioc_unlock(&(p_ioc_send_cfg->ioc));
	if (ioc_ref_send_status == OS_STATUS_OK)
	{
		ioc_ref_send_status = os_ioc_call_callbacks(&(p_ioc_send_cfg->callbacks));
	}

	return ioc_ref_send_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_ref_receive (IocIdType ioc_id, void *data)
{
	os_ioc_internal_return_type ioc_ref_receive_status = OS_IOC_E_INTERNAL_OK;
	const os_ioc_ref_receive_cfg_t  *p_ioc_recv_cfg;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	if (NULL_PTR == data) /* 入参为空指针 */
	{
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (TRUE == os_interrupt_is_disabled()) /* 关闭中断后不可调用服务 */
	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_recv_cfg = (const os_ioc_ref_receive_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_recv_cfg) ||
		 (NULL == p_ioc_recv_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_recv_cfg->send) ||
		 (NULL == p_ioc_recv_cfg->send->ioc.obj_desc) )
	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_recv_cfg->ioc.accessing_appl_mask)) )
	{
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_ioc_lock(&(p_ioc_recv_cfg->ioc));

	ioc_ref_receive_status = os_ioc_ref_receive_internal(p_ioc_recv_cfg, data);

	if (ioc_ref_receive_status == OS_IOC_E_INTERNAL_CLOSE_CHANNEL) {
		os_ioc_close_channel(&(p_ioc_recv_cfg->ioc));
		ioc_ref_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
	} else if (ioc_ref_receive_status == OS_IOC_E_INTERNAL_CHANNEL_CLOSED) {
		ioc_ref_receive_status = OS_IOC_E_INTERNAL_NO_DATA;
	} else {
		/* Do nothing */
	}
	os_ioc_unlock(&(p_ioc_recv_cfg->ioc));
	return (Os_StatusType) ioc_ref_receive_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_ref_empty_queue (IocIdType ioc_id)
{
	Os_StatusType ioc_ref_empty_queue_status = OS_STATUS_NOT_OK;
	const os_ioc_ref_receive_cfg_t   *p_ioc_recv_cfg;
	os_ioc_ref_receive_inst_t  *p_ioc_ref_recv_inst;
	const os_ioc_ref_send_inst_t  *p_ioc_ref_send_inst;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}
	if (TRUE == os_interrupt_is_disabled()) /* 关闭中断后不可调用服务 */
	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_recv_cfg = (const os_ioc_ref_receive_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ( (NULL == p_ioc_recv_cfg) ||
		 (NULL == p_ioc_recv_cfg->ioc.obj_desc) ||
		 (NULL == p_ioc_recv_cfg->send) ||
		 (NULL == p_ioc_recv_cfg->send->ioc.obj_desc) )
	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_recv_cfg->ioc.accessing_appl_mask)) )
	{
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	p_ioc_ref_recv_inst = (os_ioc_ref_receive_inst_t *)(p_ioc_recv_cfg->ioc.obj_desc);
	p_ioc_ref_send_inst = (os_ioc_ref_send_inst_t *)(p_ioc_recv_cfg->send->ioc.obj_desc);
	os_fifo_empty_queue(&(p_ioc_ref_send_inst->write_idx), &(p_ioc_ref_recv_inst->read_idx));
	ioc_ref_empty_queue_status = OS_STATUS_OK;

	return ioc_ref_empty_queue_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_write (IocIdType ioc_id, void const *data)
{
	Os_StatusType ioc_write_status = OS_STATUS_OK;
	const os_ioc_write_cfg_t *p_ioc_write_cfg;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	if (NULL_PTR == data) /* 入参为空指针 */
	{
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (TRUE == os_interrupt_is_disabled()) /* 关闭中断后不可调用服务 */
	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_write_cfg = (const os_ioc_write_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ((NULL == p_ioc_write_cfg) || (NULL == p_ioc_write_cfg->ioc.obj_desc))
	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_write_cfg->ioc.accessing_appl_mask)) )
	{
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_ioc_lock(&(p_ioc_write_cfg->ioc));

	p_ioc_write_cfg->copy_cbk(data, p_ioc_write_cfg->data);

	os_ioc_unlock(&(p_ioc_write_cfg->ioc));

	ioc_write_status = os_ioc_call_callbacks(&(p_ioc_write_cfg->callbacks));

	return ioc_write_status;
}

FUNC(Os_StatusType, OS_CODE) os_ioc_read (IocIdType ioc_id, void *data)
{
	Os_StatusType ioc_read_status = OS_STATUS_OK;
	const os_ioc_read_cfg_t  *p_ioc_read_cfg;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	if (ioc_id >= os_ioc_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	if (NULL_PTR == data) /* 入参为空指针 */
	{
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (TRUE == os_interrupt_is_disabled()) /* 关闭中断后不可调用服务 */
	{
		return OS_STATUS_DISABLEDINT;
	}
	p_ioc_read_cfg = (const os_ioc_read_cfg_t *)os_ioc_cfg_ref_table[ioc_id];

	if ((NULL == p_ioc_read_cfg) || (NULL == p_ioc_read_cfg->ioc.obj_desc))
	{
		return OS_STATUS_ID_1;
	}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if ( (OS_STATUS_OK != get_task_app_id(&app_id)) ||
		 (OS_STATUS_OK != os_app_check_access(app_id, p_ioc_read_cfg->ioc.accessing_appl_mask)) )
	{
		return OS_STATUS_ACCESSRIGHTS_1;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_ioc_lock(&(p_ioc_read_cfg->ioc));

	p_ioc_read_cfg->copy_cbk(p_ioc_read_cfg->data, data);

	os_ioc_unlock(&(p_ioc_read_cfg->ioc));

	ioc_read_status = OS_STATUS_OK;

	return ioc_read_status;
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
