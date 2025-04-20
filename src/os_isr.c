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
#include <nuttx/config.h>
#include "os_isr.h"
#include "os_core.h"
#include <sched/sched.h>
#include "os_context_internal.h"

#define OS_START_SEC_CODE
#include "os_memmap.h"

static FUNC(int, OS_CODE) os_isr_run(int irq, void *cxt, void *arg)
{
	UNUSED(irq);
	UNUSED(cxt);
	const os_isr_cfg_t *isr_cfg = (os_isr_cfg_t *)arg;
	os_core_cfg_t *core = NULL_PTR;

	core = (os_core_cfg_t *)OS_ISR_GET_CORE_REF(isr_cfg);

	/*  Autosar context 嵌套层数 + 1 */
	(void)os_context_nest_inc(OS_CORE_GET_CONTEXTNEST_REF(core), os_context_get_contexthdl());
	os_context_set_contexthdl(OS_ISR_GET_CONTEXT_REF(isr_cfg));

	isr_cfg->context.arch_ctx_cfg.entry_addr(NULL_PTR);

	/*  Autosar context 嵌套层数 - 1 */
	(void)os_context_nest_dec(OS_CORE_GET_CONTEXTNEST_REF(core), NULL_PTR);

	return 0;
}

FUNC(void, OS_CODE) os_isr_init(const os_isr_cfg_t *const os_isr_cfg_table[], OsObjIndexType size)
{
	const os_isr_cfg_t *isr_cfg;
	os_isr_inst_t *isrInst;
	OsObjIndexType index;
	const os_ctx_cfg_t *ctx_cfg;
	const os_arch_isr_cfg_t *arch_ctx_cfg;
	CoreIdType core_id = os_get_core_id();

	if ((os_isr_cfg_table == NULL) || (size == (OsObjIndexType)0)) {
		return;
	}

	for (index = 0; index < size; index++) {
		isr_cfg = os_isr_cfg_table[index];
		if (isr_cfg == NULL) {
			continue;
		}
		ctx_cfg = OS_ISR_GET_CONTEXT_REF(isr_cfg);
		if ((OS_CONTEXT_GET_OWNER_COREID(ctx_cfg) != core_id)) {
			continue;
		}
		arch_ctx_cfg = &(isr_cfg->arch_isr_cfg);
		isrInst = OS_ISR_GET_INSTANCE_REF(isr_cfg);
		isrInst->top_of_stack = NULL_PTR;
		os_context_init_context_instance(ctx_cfg);
		irq_attach((int)(arch_ctx_cfg->irq), os_isr_run, (void *)isr_cfg);
		up_init_irq((int)(arch_ctx_cfg->vector_num), (int)(arch_ctx_cfg->level));
		if (isr_cfg->is_enabled_on_init) {
			up_enable_irq((int)(arch_ctx_cfg->vector_num));
		}
	}
}

#define OS_STOP_SEC_CODE
#include "os_memmap.h"
