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

#ifndef RT_FRAMEWORK_INC_OS_IOC_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_IOC_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_hook_types.h"
#include "rtfw_ioc_types.h"

/* Global Macro Definition */
#define OS_IOC_EXTENDED_STATUS_TYPE(Status)                                                                            \
	((uint32)(((uint32)(Status)) << (sizeof(Std_ReturnType) * 8u)) | (uint32)0xFF)

/* Global Enumeration Definition */
typedef enum
{
	/*! Lock nothing. */
	OS_IOCLOCKMODE_NONE,
	/*! Lock interrupts. */
	OS_IOCLOCKMODE_INTERRUPTS,
#if (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	/*! Lock spinlocks. */
	OS_IOCLOCKMODE_SPINLOCKS,
	/*! Lock interrupts and spinlocks. */
	OS_IOCLOCKMODE_ALL
#endif /*CONFIG_RT_FRAMEWORK_SPINLOCK == 1*/
} os_ioc_lock_mode_type;

/*! An IOC channel's state. */
typedef enum
{
	/*! The channel may not be used. */
	OS_IOCCHANNELSTATE_DISABLED,
	/*! The channel may be used. */
	OS_IOCCHANNELSTATE_ENABLED
} os_ioc_channel_state_type;

/*! Class type specifier */
typedef enum
{
	ioc_08_receive_class,         /*!< Sub class is os_ioc_08_receive         */
	ioc_08_send_class,            /*!< Sub class is os_ioc_08_send            */
	ioc_16_receive_class,         /*!< Sub class is os_ioc_16_receive         */
	ioc_16_send_class,            /*!< Sub class is os_ioc_16_send            */
	ioc_32_receive_class,         /*!< Sub class is os_ioc_32_receive         */
	ioc_32_send_class,            /*!< Sub class is os_ioc_32_send            */
	ioc_ref_receive_class,        /*!< Sub class is os_ioc_ref_receive        */
	ioc_ref_send_class,           /*!< Sub class is os_ioc_ref_send           */
	ioc_read_class,               /*!< Sub class is os_ioc_read               */
	ioc_write_class               /*!< Sub class is os_ioc_write              */
} os_ioc_sub_class_type;

typedef enum
{
	OS_FIFOWRITERESULT_OK,                   /*!< Writing to the queue succeeded. */
	OS_FIFOWRITERESULT_BUFFEROVERFLOW        /*!< Writing to the queue failed, because it is full. */
} os_fifo_write_result_type;

/*! Internal return types for receive functions (more error codes than the API functions) */
typedef enum
{
	/*! No error. */
	OS_IOC_E_INTERNAL_OK                  = E_OK,
	/*! The calling application is not the owner application of the send/receive function. */
	OS_IOC_E_INTERNAL_NOK                 = E_NOT_OK,
	/*! Queue is empty, return value is empty. */
	OS_IOC_E_INTERNAL_NO_DATA             = E_OS_IOC_NO_DATA,
	/*! Queue overflow detected, data lost, receive successful. */
	OS_IOC_E_INTERNAL_LOST_DATA           = E_OS_IOC_LOST_DATA,
	/*! Queue overflow detected, data lost and queue is currently empty, receive successful. */
	OS_IOC_E_INTERNAL_LOST_AND_NO_DATA    = (E_OS_IOC_NO_DATA | E_OS_IOC_LOST_DATA),
	/*! Channel is inactive because of an earlier detected internal error. */
	OS_IOC_E_INTERNAL_CHANNEL_CLOSED      = OS_IOC_EXTENDED_STATUS_TYPE(1u),
	/*! Channel shall be closed because of internal error (e.g. read/write-index out of range). */
	OS_IOC_E_INTERNAL_CLOSE_CHANNEL       = OS_IOC_EXTENDED_STATUS_TYPE(2u)
} os_ioc_internal_return_type;

/* Global Type & Structure Definition */
typedef uint8 os_ioc_lost_counter_type;
typedef uint8 os_fifo_idx_type;

typedef os_spin_id_t os_spin_cfg_ref_t;
typedef os_hook_cfg_t os_ioc_call_back_ref_t;

typedef void (* os_ioc_copy_cbk) (void const *source, void *target);
typedef void (* os_ioc_init_cbk) (void *data);
typedef void (* os_fifo_ref_write_cbk) (void *buffer, uint8 idx, void const *data);
typedef void (* os_fifo_ref_read_cbk) (void const *buffer, uint8 idx, void *data_ref);

struct os_ioc_callbacks_config_s {
	uint32 callback_count;
	const os_ioc_call_back_ref_t * const *callback_refs;
};

struct os_ioc_inst_s {
	os_ioc_channel_state_type state;
};

struct os_ioc_cfg_s {
	os_ioc_inst_t *obj_desc;
	IocIdType ioc_id;
	os_ioc_sub_class_type sub_class;
	os_ioc_lock_mode_type lock_mode;
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	uint32 accessing_appl_mask;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
#if (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	os_spin_cfg_ref_t spin_id;
#endif
};

struct os_ioc_08_send_inst_s {
	os_ioc_inst_t base;
	os_fifo_idx_type write_idx;
	os_ioc_lost_counter_type lost_counter;
};

struct os_ioc_08_send_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_08_receive_cfg_t *receive;
	os_ioc_callbacks_config_t callbacks;
	uint8 *data;
	uint8 size;
};

struct os_ioc_16_send_inst_s {
	os_ioc_inst_t base;
	os_fifo_idx_type write_idx;
	os_ioc_lost_counter_type lost_counter;
};

struct os_ioc_16_send_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_16_receive_cfg_t *receive;
	os_ioc_callbacks_config_t callbacks;
	uint16 *data;
	uint8 size;
};

struct os_ioc_32_send_inst_s {
	os_ioc_inst_t base;
	os_fifo_idx_type write_idx;
	os_ioc_lost_counter_type lost_counter;
};

struct os_ioc_32_send_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_32_receive_cfg_t *receive;
	os_ioc_callbacks_config_t callbacks;
	uint32 *data;
	uint8 size;
};

struct os_ioc_ref_send_inst_s {
	os_ioc_inst_t base;
	os_fifo_idx_type write_idx;
	os_ioc_lost_counter_type lost_counter;
};

struct os_ioc_ref_send_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_ref_receive_cfg_t *receive;
	os_fifo_ref_write_cbk write_cbk;
	os_ioc_callbacks_config_t callbacks;
	void *data;
	uint8 size;
};

struct os_ioc_write_inst_s {
	os_ioc_inst_t base;
};

struct os_ioc_write_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_read_cfg_t *read;
	void *data;
	os_ioc_copy_cbk copy_cbk;
	os_ioc_init_cbk init_cbk;
	os_ioc_callbacks_config_t callbacks;
};

struct os_ioc_08_receive_inst_s {
	os_ioc_inst_t base;
	os_fifo_idx_type read_idx;
	os_ioc_lost_counter_type lost_counter_snapshot;
};

struct os_ioc_08_receive_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_08_send_cfg_t *send;
	uint8 *data;
	uint8 size;
};

struct os_ioc_16_receive_inst_s {
	os_ioc_inst_t base;
	os_fifo_idx_type read_idx;
	os_ioc_lost_counter_type lost_counter_snapshot;
};

struct os_ioc_16_receive_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_16_send_cfg_t *send;
	uint16 *data;
	uint8 size;
};

struct os_ioc_32_receive_inst_s {
	os_ioc_inst_t base;
	os_fifo_idx_type read_idx;
	os_ioc_lost_counter_type lost_counter_snapshot;
};

struct os_ioc_32_receive_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_32_send_cfg_t *send;
	uint32 *data;
	uint8 size;
};

struct os_ioc_ref_receive_inst_s {
	os_ioc_inst_t base;
	os_fifo_idx_type read_idx;
	os_ioc_lost_counter_type lost_counter_snapshot;
};

struct os_ioc_ref_receive_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_ref_send_cfg_t *send;
	os_fifo_ref_read_cbk read_cbk;
	void *data;
	uint8 size;
};

struct os_ioc_read_inst_s {
	os_ioc_inst_t base;
};

struct os_ioc_read_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_write_cfg_t *write;
	void *data;
	os_ioc_copy_cbk copy_cbk;
};

/* Global Variable Definition */
DeclareRtfwRefTable_H(ioc)  /*! Reference to OS iocchannels configuration table  */

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
extern void os_ioc_init(const os_ioc_cfg_t *const os_ioc_cfg_table[], OsObjIndexType size);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_08_send (IocIdType ioc_id, uint8 data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_08_receive (IocIdType ioc_id, uint8 *data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_08_empty_queue (IocIdType ioc_id);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_16_send (IocIdType ioc_id, uint16 data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_16_receive (IocIdType ioc_id, uint16 *data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_16_empty_queue (IocIdType ioc_id);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_32_send (IocIdType ioc_id, uint32 data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_32_receive (IocIdType ioc_id, uint32 *data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_32_empty_queue (IocIdType ioc_id);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_ref_send (IocIdType ioc_id, void const *data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_ref_receive (IocIdType ioc_id, void *data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_ref_empty_queue (IocIdType ioc_id);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_write (IocIdType ioc_id, void const *data);
extern FUNC(Os_StatusType, OS_CODE) os_ioc_read (IocIdType ioc_id, void *data);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_IOC_INTERNAL_H */
