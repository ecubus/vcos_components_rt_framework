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

#ifndef RT_FRAMEWORK_INC_RTFW_TYPES_H
#define RT_FRAMEWORK_INC_RTFW_TYPES_H

#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <nuttx/event.h>
#include "Os_Types.h"
#include <stddef.h>
#include <stdint.h>

typedef int16_t isr_id_t;

typedef nxevent_mask_t os_event_mask_t;

/* Global Type & Structure Definition */
typedef uintptr_t int_global_state_t; /* Interrupt global state type */
typedef uintptr_t int_vector_num_t; /* Interrupt vector number */
typedef uintptr_t register_t; /* Register type */
typedef uint16_t os_spin_id_t;

typedef int_global_state_t os_int_global_state_t;
typedef uint16_t os_int_api_cnt_t; // TODO 性能优化可以变为 uint32

typedef uint32_t os_context_id_t;

typedef TryToGetSpinlockType try_get_spin_t;

#define RTFW_OFFSETOF(type, member) offsetof(type, member) ///< 获取结构体成员的偏移量

#define NELEMENTS(array) (sizeof(array) / sizeof((array)[0])) ///< 获取数组元素的个数

#define RTFW_APPID2MASK(appid) (0x00000001U << (appid)) ///< 获取应用程序ID掩码

#define RTFW_LINK_ENTRY_NAME(_class, _id) _class##_##_id

#define RTFW_LINK_ENTRY_DECLARE(_section, _class, _type, _id)                                                          \
	_type RTFW_LINK_ENTRY_NAME(_class, _id) locate_data(_section)

#define RTFW_TABLE_DECLARE(_section, _class, _type, _id, _size)                                                        \
	_type RTFW_LINK_ENTRY_NAME(_class, _id)[(_size) / sizeof(_type)] locate_data(_section)

#define RTFW_TABLE_DECLARE_BY_ARRAY_MEMBER(_section, _class, _type, _id, _num)                                         \
	_type RTFW_LINK_ENTRY_NAME(_class, _id)[_num] locate_data(_section)

#define DeclareRtfwRefTable(_section, _class, _size)                                                                   \
	const locate_data(_section) OsObjIndexType os_##_class##_cfg_ref_table_size = (_size);                           \
	const locate_data(_section) os_##_class##_cfg_t *const os_##_class##_cfg_ref_table[(_size)]

#define DeclareRtfwRefTable_H(_class)                                                                                  \
	extern const OsObjIndexType os_##_class##_cfg_ref_table_size;                                                  \
	extern const os_##_class##_cfg_t *const os_##_class##_cfg_ref_table[];

#endif /* RT_FRAMEWORK_INC_RTFW_TYPES_H */