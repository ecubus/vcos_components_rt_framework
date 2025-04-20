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

#ifndef AUTOSAROS_INC_OS_EVENT_INTERNAL_H
#define AUTOSAROS_INC_OS_EVENT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include "Os_Types.h"

/* Global Variable Definition */

/* Global Function Declaration */

#define OS_START_SEC_CODE
#include "os_memmap.h"

extern FUNC(Os_StatusType, OS_CODE) os_set_event(TaskType task_id, os_event_mask_t mask);
extern FUNC(Os_StatusType, OS_CODE) os_set_event_asyn(TaskType task_id, os_event_mask_t mask);
extern FUNC(Os_StatusType, OS_CODE) os_clear_event(os_event_mask_t mask);
extern FUNC(Os_StatusType, OS_CODE) os_wait_event(os_event_mask_t mask);
extern FUNC(Os_StatusType, OS_CODE) os_get_event(TaskType task_id, os_event_mask_t *mask);

#define OS_STOP_SEC_CODE
#include "os_memmap.h"

/* Global Function Implementation */

#ifdef __cplusplus
}
#endif

#endif /* AUTOSAROS_INC_OS_EVENT_INTERNAL_H */
