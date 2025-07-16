############################################################################
#  Copyright (c) 2025 Li Auto Inc. and its affiliates
#  Licensed under the Apache License, Version 2.0(the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
############################################################################

from components.rt_framework.config_plug_in.generator import utils
from func_module.generator.common.common import FunctionBase
from func_module.generator.common.user_code_extract import UserCodeExtract
from components.memory.mempool.config_plug_in.generator.utils import get_os_mempool_all_config_dict
from jinja2 import pass_context
from decimal import Decimal
from collections import defaultdict


class Function(FunctionBase):
    """
    Class containing all functions. Add new static functions here
    to be included in the jinja2 context
    """
    _cache = []

    @staticmethod
    def clear_caches():
        for ch in Function._cache:
            ch.clear_cache()

    @staticmethod
    @pass_context
    def get_os_module_list(context, module_path):
        """get os module list"""
        return utils.get_module_list(context['os_ui'], module_path)

    @staticmethod
    @pass_context
    def get_access_app_mask(context, app_set, index_size):
        app_mask = ""
        indent = " " * index_size
        app_set = sorted(list(set(app_set)))
        app_cfg = Function.get_app_cfg(context)
        for app in app_set:
            if app != '':
                app_mask = app_mask + '|\n' + indent + "RTFW_APPID2MASK(%s)" % (app_cfg[app]["AppId"])
        return app_mask[2:].lstrip()

    @staticmethod
    @pass_context
    def get_core_configs(context):
        core_cfg = context["os_core"]
        return core_cfg

    @staticmethod
    @pass_context
    def get_core_cnt(context):
        core_cfg = context["os_core"]
        return len(core_cfg)

    @staticmethod
    @pass_context
    def get_hw2log_map(context):
        core_cfg = context["os_core"]
        hwcnt = len(core_cfg)
        hw2log_map = []
        for hw_cid in range(hwcnt):
            for cval in core_cfg.values():
                if int(cval['PhyId'].split('_')[-1]) == hw_cid:
                    sw_cid = cval['core_id'].split('_')[-1]
                    hw2log_map.append(f'OS_CORE_ID_{sw_cid}')
                    break
        return hw2log_map

    @staticmethod
    @pass_context
    def get_log2hw_map(context):
        core_cfg = context["os_core"]
        logcnt = len(core_cfg)
        log2hw_map = []
        for sw_cid in range(logcnt):
            for cval in core_cfg.values():
                if int(cval['core_id'].split('_')[-1]) == sw_cid:
                    hw_cid = cval['PhyId'].split('_')[-1]
                    log2hw_map.append(f'OS_CORE_PHYSICAL_ID_{hw_cid}')
                    break
        return log2hw_map

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_task_cfg(context):
        task_dict = Function.get_os_module_list(context, '/Os/OsTask')
        all_task_cfg = {}

        for name, value in task_dict.items():
            task_cfg = {
                "TaskType": value["OsTaskType"]["Value"],
                "TaskPriority": value["OsTaskPriority"]["Value"],
                "TaskActivation": value["OsTaskActivation"]["Value"],
                "TaskStackSize": value["OsTaskStackSize"]["Value"],
                "TaskStackSection": f'OS_{name.upper()}_STACK_SECTION',
                "TaskAccessApp": utils.resolve_ref_list("OsTaskAccessingApplication", value),
                "TaskEventRef": utils.resolve_ref_list("OsTaskEventRef", value),
                "TaskResourceRef": utils.resolve_ref_list("OsTaskResourceRef", value),
                "TaskUsesFpu": 1 if value["OsTaskUsesFpu"]["Value"] else 0,
                "TaskAutostart": "FALSE"
            }

            if "OsTaskAutostart" in value:
                task_auto_start = utils.get_last_item(value["OsTaskAutostart"]["OsTaskAppModeRef"]["ValueRef"])
                task_cfg["TaskAutostart"] = "TRUE" if task_auto_start == "OSAUTOSTARTMODE" else "FALSE"

            all_task_cfg[name] = task_cfg

        return all_task_cfg

    @staticmethod
    @pass_context
    def get_bsw_app_from_coreid(context, coreid):
        app_cfg = Function.get_app_cfg(context)
        for cfg in app_cfg.values():
            if int(cfg["CoreNumber"]) == int(coreid):
                return cfg["BswAppName"]
        return "Unknwon"

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_app_cfg(context):
        app_dict = Function.get_os_module_list(context, '/Os/OsApplication')
        all_app_cfg = {}

        # solve the basic information about application config
        for name, value in app_dict.items():
            app_coreid = value["OsApplicationCoreRef"]["ValueRef"][-1]

            app_cfg = {
                "CoreNumber": app_coreid,
                "AppMemoryProtectionIdentifier": value["OsAppMemoryProtectionIdentifier"]["Value"],
                "AppTaskList": utils.resolve_ref_list("OsAppTaskRef", value),
                "AppTaskTable": name.lower() + "_task_cfg_ref_table",
                "AppCounterList": utils.resolve_ref_list("OsAppCounterRef", value),
                "AppCounterTable": name.lower() + "_counter_cfg_ref_table",
                "AppIsrList": utils.resolve_ref_list("OsAppIsrRef", value),
                "AppIsrTable": name.lower() + "_isr_cfg_ref_table",
                "AppScheduleTableList": utils.resolve_ref_list("OsAppScheduleTableRef", value),
                "AppSchedTblTable": name.lower() + "_schedtable_cfg_ref_table",
                "AppHookTable": name.lower() + "_hook_cfg_ref_table",
                "AppPrivileged": "TRUE" if value["OsIsPrivileged"]["Value"] else "FALSE",
                "AppId": f"OS_CORE{app_coreid}_{name.upper()}_ID",
                "AppInstSection": f"OS_{name.upper()}_BSS_SECTION",
                "AppStartupHook": False,
                "AppErrorHook": False,
                "AppShutdownHook": False,
                "AppHookCnt": 0
            }

            app_hooks = value.get("OsApplicationHooks", {})
            if app_hooks:
                if app_hooks["OsAppStartupHook"]['Value'] == True:
                    app_cfg["AppStartupHook"] = True
                    app_cfg["AppHookCnt"] += 1
                if app_hooks["OsAppErrorHook"]['Value'] == True:
                    app_cfg["AppErrorHook"] = True
                    app_cfg["AppHookCnt"] += 1
                if app_hooks["OsAppShutdownHook"]['Value'] == True:
                    app_cfg["AppShutdownHook"] = True
                    app_cfg["AppHookCnt"] += 1

            all_app_cfg[name] = app_cfg

        # get the mapping between coreid and bsw app
        core_bsw_app_dict = {}
        cores_list = Function.get_os_module_list(context, "/Os/OsCore")
        for value in cores_list.values():
            bsw_appname = utils.get_last_item(value["OsCoreBswApplicationRef"]["ValueRef"])
            core_bsw_app_dict[all_app_cfg[bsw_appname]["CoreNumber"]] = bsw_appname

        # find the bsw app for every applition
        for name, value in app_dict.items():
            all_app_cfg[name]["BswAppName"] = core_bsw_app_dict[all_app_cfg[name]["CoreNumber"]]

        return all_app_cfg

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_isr_cfg(context):
        isr_dict = Function.get_os_module_list(context, "/Os/OsIsr")
        all_isr_cfg = {}

        for name, value in isr_dict.items():
            isr_cfg = {
                'IsrEnableOnInit': "FALSE",
                'IsrIntPriority': value["OsIsrInterruptPriority"]["Value"],
                'IsrIntSource': value["OsIsrInterruptSource"]["Value"],
                'IsrCategory': 'ISR_CAT0',
                'IsrContext': 'OS_ISR0_CONTEXT_ID',
                'IsrEntryFunc': value["OsIsrEntryFunctionName"]["Value"],
                'IsrEntryParam': 'NULL_PTR'
            }

            if value["OsIsrInitialEnableInterruptSource"]["Value"]:
                isr_cfg["IsrEnableOnInit"] = "TRUE"

            if value["OsIsrCategory"]["Value"] == "CATEGORY_2":
                isr_cfg["IsrCategory"] = "ISR_CAT2"
                isr_cfg["IsrContext"] = "OS_ISR2_CONTEXT_ID"
            elif value["OsIsrCategory"]["Value"] == "CATEGORY_1":
                isr_cfg["IsrCategory"] = "ISR_CAT1"
                isr_cfg["IsrContext"] = "OS_ISR1_CONTEXT_ID"
            elif value["OsIsrCategory"]["Value"] == "CATEGORY_TP":
                isr_cfg["IsrCategory"] = "ISR_TP"
                isr_cfg["IsrContext"] = "OS_TPISR_CONTEXT_ID"

            all_isr_cfg[name] = isr_cfg

        return all_isr_cfg

    @staticmethod
    def solve_spinlock_order(spinlock_cfg):
        successor_set = []
        # find all successors
        for spin_val in spinlock_cfg.values():
            if spin_val["Successor"]:
                successor_set.append(spin_val["Successor"][0])

        # initialise spinlock orders
        successor_set = set(successor_set)
        for spin, spin_val in spinlock_cfg.items():
            if spin not in successor_set:
                spin_val["Order"] = 0

        # update OsSpinlockOrder resolve order iteratively
        # if current_order didn't increase in an iteration,
        # it means ring exists.
        current_order = 0
        updated = True
        while updated:
            updated = False
            for spinlock_val in spinlock_cfg.values():
                if spinlock_val["Successor"] and spinlock_val["Order"] == current_order:
                    succ = spinlock_val["Successor"][0]
                    if spinlock_cfg[succ]["Order"] == -1:
                        spinlock_cfg[succ]["Order"] = current_order + 1
                        updated = True
            current_order += 1

        # set spinlocks in a loop to order == 0
        for spin_val in spinlock_cfg.values():
            if spin_val["Order"] == -1:
                spin_val["Order"] = 0

    @staticmethod
    @pass_context
    def get_spinlock_cfg(context):
        spinlock_cfg = {}
        spinlock_list = Function.get_os_module_list(context, "/Os/OsSpinlock")
        for spin, spin_val in spinlock_list.items():
            cfg_i = {}
            cfg_i["Order"] = -1
            cfg_i["Method"] = spin_val["OsSpinlockLockMethod"]["Value"]

            if spin_val["OsSpinlockLockType"]["Value"] == "OPTIMIZED":
                cfg_i["LockType"] = "SPINLOCKCHECK_DISABLED"
            else:
                cfg_i["LockType"] = "SPINLOCKCHECK_ENABLED"

            cfg_i["Successor"] = utils.resolve_ref_list('OsSpinlockSuccessor', spin_val)

            cfg_i["AccessingApplication"] = \
                utils.resolve_ref_list('OsSpinlockAccessingApplication', spin_val)
            spinlock_cfg[spin] = cfg_i

        Function.solve_spinlock_order(spinlock_cfg)

        return utils.sort_dict_by_key(spinlock_cfg)

    @staticmethod
    @pass_context
    def get_resource_referenced_task(context, resource_name):
        """
        返回引用该Resource的所有Task的(task_name, priority)元组列表
        """
        task_cfg = Function.get_task_cfg(context)
        referenced = []
        for task_name, task_val in task_cfg.items():
            res_refs = task_val.get("TaskResourceRef", [])
            if isinstance(res_refs, str):
                res_refs = [res_refs]
            for res_ref in res_refs:
                res_name = utils.get_last_item(res_ref)
                if res_name == resource_name:
                    prio = int(task_val.get("TaskPriority", 0))
                    referenced.append((task_name, prio))
        return referenced

    @staticmethod
    @pass_context
    def get_resource_cfg(context):
        resource_cfg = {}
        resource_list = Function.get_os_module_list(context, "/Os/OsResource")
        app_cfg = Function.get_app_cfg(context)
        for res_name, res_val in resource_list.items():
            cfg_i = {}
            cfg_i["OsResourceAccessingApplication"] = utils.resolve_ref_list("OsResourceAccessingApplication", res_val)
            core_id = '0'
            if cfg_i["OsResourceAccessingApplication"]:
                app_name = cfg_i["OsResourceAccessingApplication"][0]
                core_id = Function.get_coreid_by_app_name(context, app_name)
            cfg_i["CoreID"] = core_id
            # CeilingPriority
            referenced_tasks = Function.get_resource_referenced_task(context, res_name)
            if referenced_tasks:
                cfg_i["CeilingPriority"] = max(prio for _, prio in referenced_tasks) + 1
            else:
                cfg_i["CeilingPriority"] = 1
            resource_cfg[res_name] = cfg_i
        return utils.sort_dict_by_key(resource_cfg)

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_core_referenced_resource(context):
        """
        Get resource list referenced by each core
        Returns a dict: {core_id: [resource_name_list]}
        """
        resource_cfg = Function.get_resource_cfg(context)
        core_resource_map = {}

        # Initialize empty resource list for all cores
        core_cnt = Function.get_core_cnt(context)
        for core_id in range(core_cnt):
            core_resource_map[str(core_id)] = []

        # Assign resources to corresponding cores
        for resource_name, resource_info in resource_cfg.items():
            core_id = resource_info["CoreID"]
            if core_id in core_resource_map:
                core_resource_map[core_id].append(resource_name)

        return core_resource_map

    @staticmethod
    @pass_context
    def get_event_cfg(context):
        event_cfg = {}
        task_cfg = Function.get_task_cfg(context)
        event_dict = Function.get_os_module_list(context, "/Os/OsEvent")

        # only event referenced by task
        # will be generated.
        for task_key, task_value in task_cfg.items():
            task_event_cfg = {}
            mask_set = set()
            task_event_list = task_value['TaskEventRef']
            for ev in task_event_list:
                ev_mask = event_dict[ev]["OsEventMask"]["Value"]
                if ev_mask is not None and ev_mask != "":
                    task_event_cfg[ev] = int(ev_mask)
                    mask_set.add(int(ev_mask))
                else:
                    task_event_cfg[ev] = -1

            # assign event mask
            # avoid conflicting the event mask set by user
            i = 0
            for ename, emask in task_event_cfg.items():
                if emask == -1:
                    while i in mask_set:
                        i += 1
                    task_event_cfg[ename] = i
                    mask_set.add(i)
                    i += 1

            # save the task's event cfg to
            # all event cfg dict.
            event_cfg[task_key] = task_event_cfg

        event_cfg = utils.sort_dict_by_key(event_cfg)
        event_cfg = {k: utils.sort_dict_by_key(v) for k, v in event_cfg.items()}

        return event_cfg

    @staticmethod
    @pass_context
    def get_target(context):
        target = context['sys_cfg']['target']
        return target

    @staticmethod
    @pass_context
    def check_isq_dispatch_method_is_level(context):
        target = Function.get_target(context)
        if target in ("AURIX2G_TC397"):
            return True
        elif target in ("E3650", "J6E"):
            return False
        else:
            raise TypeError("没有适配的架构！")

    @staticmethod
    @pass_context
    def get_isr_counter(context):
        isr_counter = {}
        counter_list = Function.get_os_module_list(context, "/Os/OsCounter")
        for counter_name, counter_value in counter_list.items():
            if "OsDriver" in counter_value:
                isr_name = utils.get_last_item(counter_value["OsDriver"]["OsIsrRef"]["ValueRef"])
                isr_counter.update({isr_name : counter_name})
        return isr_counter

    @staticmethod
    @pass_context
    def get_counter_cfg(context):
        counter_list = Function.get_os_module_list(context, "/Os/OsCounter")
        app_cfg = Function.get_app_cfg(context)
        schedtable_cfg = Function.get_schedtable_cfg(context)
        counter_cfg = {}
        action_cnt = {}

        for app_name, app_value in app_cfg.items():
            for schedtable_name in app_value["AppScheduleTableList"]:
                if schedtable_name in schedtable_cfg:
                    if schedtable_cfg[schedtable_name]["OsScheduleTableCounterRef"]:
                        ref_counter_name = schedtable_cfg[schedtable_name]["OsScheduleTableCounterRef"]
                        if ref_counter_name not in action_cnt.keys():
                            action_cnt[ref_counter_name] = 0
                        action_cnt[ref_counter_name] = action_cnt[ref_counter_name] + 1

        for counter_name, counter_value in counter_list.items():
            counter_content = {}
            CounterAccessApp = []
            CounterType = counter_value["OsCounterType"]["Value"]
            CounterMaxAllowedValue = counter_value["OsCounterMaxAllowedValue"]["Value"]
            CounterMinCycle = counter_value["OsCounterMinCycle"]["Value"]
            SecondsPerTick = counter_value["OsSecondsPerTick"]["Value"]
            if CounterType == "SOFTWARE":
                TimerType = "TIMERTYPE_SOFTWARE"
                CounterTicksPerBase = 1
                IsrRef = ""
                CounterMaxInternalTickValue = 2 * CounterMaxAllowedValue + 1
            else:
                TimerType = "TIMERTYPE_HIGH_RESOLUTION"
                if CounterMaxAllowedValue != 1073741823:
                    CounterMaxAllowedValue = 1073741823
                CounterTicksPerBase = 1
                IsrRef = utils.get_last_item(counter_value["OsDriver"]["OsIsrRef"]["ValueRef"])
                CounterMaxInternalTickValue = 4 * CounterMaxAllowedValue + 3
            CounterAccessApp = utils.resolve_ref_list('OsCounterAccessingApplication', counter_value)
            if counter_name in action_cnt:
                ActionCnt = action_cnt[counter_name] + 2
            else:
                ActionCnt = 2
            TicksPerMs = round(1 / (SecondsPerTick * 1000))
            counter_content.update({"CounterAccessApp": CounterAccessApp})
            counter_content.update({"CounterMaxAllowedValue": CounterMaxAllowedValue})
            counter_content.update({"CounterMinCycle": CounterMinCycle})
            counter_content.update({"CounterType": CounterType})
            counter_content.update({"ActionCnt": ActionCnt})
            counter_content.update({"CounterTicksPerBase": CounterTicksPerBase})
            counter_content.update({"TimerType": TimerType})
            counter_content.update({"CounterMaxInternalTickValue": CounterMaxInternalTickValue})
            counter_content.update({"IsrRef": IsrRef})
            counter_content.update({"TicksPerMs": TicksPerMs})
            counter_cfg.update({counter_name: counter_content})
        return counter_cfg

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_system_counter_list(context):
        """Get system counter list for each core"""

        core_cfg = context["os_core"]
        system_counter_list = {}

        for core_id in range(len(core_cfg)):
            system_counter_list[core_id] = f"SystemTimer{core_id}"

        return system_counter_list

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_schedtable_cfg(context):
        """Get schedule table configuration information"""

        schedtable_dict = Function.get_os_module_list(context, "/Os/OsScheduleTable")
        schedtable_cfg = {}

        for name, value in schedtable_dict.items():
            schedtable_cfg[name] = {}

            # Process basic attributes
            schedtable_cfg[name]["OsScheduleTableDuration"] = value["OsScheduleTableDuration"]["Value"]

            # Handle Repeating field, accept various true values
            if "OsScheduleTableRepeating" in value:
                repeating_value = value["OsScheduleTableRepeating"].get("Value")
                repeating = repeating_value == "REPEATING" or repeating_value == True or repeating_value == "True" or repeating_value == 1 or repeating_value == "1"
                schedtable_cfg[name]["OsScheduleTableRepeating"] = repeating
            else:
                schedtable_cfg[name]["OsScheduleTableRepeating"] = False

            # Counter reference
            if "OsScheduleTableCounterRef" in value:
                schedtable_cfg[name]["OsScheduleTableCounterRef"] = utils.get_last_item(value["OsScheduleTableCounterRef"]["ValueRef"])
            else:
                schedtable_cfg[name]["OsScheduleTableCounterRef"] = ""

            # Synchronization strategy
            schedtable_cfg[name]["OsScheduleTableSync"] = {}
            if "OsScheduleTableSync" in value:
                sync_container = value["OsScheduleTableSync"]
                if "OsScheduleTblSyncStrategy" in sync_container:
                    sync_strategy = sync_container["OsScheduleTblSyncStrategy"]["Value"]
                    if sync_strategy == "EXPLICIT":
                        schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "EXPLICIT"
                    elif sync_strategy == "IMPLICIT":
                        schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "IMPLICIT"
                    else:
                        schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "NONE"
                else:
                    schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "NONE"

                # Parse explicit precision parameter
                if "OsScheduleTblExplicitPrecision" in sync_container:
                    schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblExplicitPrecision"] = sync_container["OsScheduleTblExplicitPrecision"]["Value"]
                else:
                    schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblExplicitPrecision"] = 0
            else:
                schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "NONE"
                schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblExplicitPrecision"] = 0

            # Autostart configuration
            schedtable_cfg[name]["OsScheduleTableAutostart"] = {}

            # Check if autostart type is configured
            has_autostart = "OsScheduleTableAutostartType" in value

            if has_autostart:
                autostart_type = value["OsScheduleTableAutostartType"]["Value"]
                schedtable_cfg[name]["OsScheduleTableAutostart"]["OsScheduleTableAutostartType"] = f"SCHTBL_AUTOSTART_{autostart_type}"

                # Get start value
                if "OsScheduleTableStartValue" in value:
                    schedtable_cfg[name]["OsScheduleTableAutostart"]["OsScheduleTableStartValue"] = value["OsScheduleTableStartValue"]["Value"]
                else:
                    schedtable_cfg[name]["OsScheduleTableAutostart"]["OsScheduleTableStartValue"] = 0

                # Get application mode
                if "OsScheduleTableAppModeRef" in value:
                    schedtable_cfg[name]["OsScheduleTableAutostart"]["OsScheduleTableAppModeRef"] = utils.get_last_item(value["OsScheduleTableAppModeRef"]["ValueRef"])
                else:
                    schedtable_cfg[name]["OsScheduleTableAutostart"]["OsScheduleTableAppModeRef"] = "OSDEFAULTAPPMODE"
            else:
                # If autostart is not configured, set default value to RELATIVE
                schedtable_cfg[name]["OsScheduleTableAutostart"]["OsScheduleTableAutostartType"] = "SCHTBL_AUTOSTART_RELATIVE"
                schedtable_cfg[name]["OsScheduleTableAutostart"]["OsScheduleTableStartValue"] = 0
                schedtable_cfg[name]["OsScheduleTableAutostart"]["OsScheduleTableAppModeRef"] = "OSDEFAULTAPPMODE"

            # Accessing application
            if "OsSchTblAccessingApplication" in value:
                schedtable_cfg[name]["OsSchTblAccessingApplication"] = \
                    utils.resolve_ref_list("OsSchTblAccessingApplication", value)
            else:
                schedtable_cfg[name]["OsSchTblAccessingApplication"] = []

            # Process expiry points
            schedtable_cfg[name]["OsScheduleTableExpiryPoint"] = {}

            # Collect all ExpiryPoint_X keys and their offset values
            expiry_points = {}
            expiry_point_offsets = {}

            for key, val in value.items():
                if key.startswith("ExpiryPoint_"):
                    expiry_points[key] = val
                    # Save offset value for sorting
                    if "OsScheduleTblExpPointOffset" in val:
                        expiry_point_offsets[key] = int(val["OsScheduleTblExpPointOffset"]["Value"])
                    else:
                        expiry_point_offsets[key] = 0

            # Sort ExpiryPoint keys by their offset values
            sorted_expiry_points = sorted(expiry_points.keys(), key=lambda k: expiry_point_offsets[k])

            # Process each expiry point (maintain sorted order)
            for ep_name in sorted_expiry_points:
                ep_value = expiry_points[ep_name]
                schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name] = {}

                # Offset is required
                if "OsScheduleTblExpPointOffset" in ep_value:
                    schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTblExpPointOffset"] = \
                        ep_value["OsScheduleTblExpPointOffset"]["Value"]
                else:
                    schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTblExpPointOffset"] = 0

                # Process task activations - THIS IS THE KEY ADDITION
                task_activations = []

                # Check for task activation entries - look for OsScheduleTableTaskActivation_X keys
                for ta_key, ta_value in ep_value.items():
                    if ta_key.startswith("OsScheduleTableTaskActivation_"):
                        if "OsScheduleTableActivateTaskRef" in ta_value and ta_value["OsScheduleTableActivateTaskRef"]["ValueRef"]:
                            # Get task reference from ValueRef
                            task_ref = utils.get_last_item(ta_value["OsScheduleTableActivateTaskRef"]["ValueRef"])
                            # Add to task activations list with OS_TASK_ prefix
                            task_activations.append(f"OS_TASK_{task_ref.upper()}_ID")

                # Set the task activations list
                schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTableActivateTaskRef"] = task_activations

                # Process event settings
                event_settings = []

                # Find all event setting entries for this expiry point
                for es_key, es_value in ep_value.items():
                    if es_key.startswith("OsScheduleTableEventSetting_"):
                        # Get event mask and task reference
                        if "OsScheduleTableSetEventRef" in es_value and "OsScheduleTableSetEventTaskRef" in es_value:
                            # Get event reference (last part of the path)
                            event_ref = utils.get_last_item(es_value["OsScheduleTableSetEventRef"]["ValueRef"])
                            # Convert event mask to uppercase
                            event_mask = event_ref.upper()

                            # Get task reference (last part of the path)
                            task_ref = utils.get_last_item(es_value["OsScheduleTableSetEventTaskRef"]["ValueRef"])
                            # Convert task ID to uppercase
                            task_id = task_ref.upper()

                            # Add as a tuple containing uppercase event mask and task ID
                            event_settings.append((event_mask, f"OS_TASK_{task_id}_ID"))

                schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTableSetEventRef"] = event_settings

                # Adjustable expiry point - ensure all required keys exist
                schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTblAdjustableExpPoint"] = {}

                if "OsScheduleTblAdjustableExpPoint" in ep_value:
                    adj_exp_point = ep_value["OsScheduleTblAdjustableExpPoint"]

                    # Ensure MaxShorten exists
                    if "OsScheduleTableMaxShorten" in adj_exp_point:
                        schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTblAdjustableExpPoint"]["OsScheduleTableMaxShorten"] = \
                            adj_exp_point["OsScheduleTableMaxShorten"]["Value"]
                    else:
                        schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTblAdjustableExpPoint"]["OsScheduleTableMaxShorten"] = 0

                    # Ensure MaxLengthen exists
                    if "OsScheduleTableMaxLengthen" in adj_exp_point:
                        schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTblAdjustableExpPoint"]["OsScheduleTableMaxLengthen"] = \
                            adj_exp_point["OsScheduleTableMaxLengthen"]["Value"]
                    else:
                        schedtable_cfg[name]["OsScheduleTableExpiryPoint"][ep_name]["OsScheduleTblAdjustableExpPoint"]["OsScheduleTableMaxLengthen"] = 0

        return schedtable_cfg

    @staticmethod
    @pass_context
    def get_schedtable_sync_strategy(context, schedtable_cfg, schedtable_name):
        """Get schedule table synchronization strategy"""
        if ("OsScheduleTableSync" in schedtable_cfg[schedtable_name] and
            "OsScheduleTblSyncStrategy" in schedtable_cfg[schedtable_name]["OsScheduleTableSync"]):
            return schedtable_cfg[schedtable_name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"]
        else:
            return "NONE"

    @staticmethod
    @pass_context
    def get_schedtable_sync_precision(context, schedtable_cfg, schedtable_name):
        """Get schedule table synchronization precision"""
        if ("OsScheduleTableSync" in schedtable_cfg[schedtable_name] and
            "OsScheduleTblExplicitPrecision" in schedtable_cfg[schedtable_name]["OsScheduleTableSync"]):
            return schedtable_cfg[schedtable_name]["OsScheduleTableSync"]["OsScheduleTblExplicitPrecision"]
        else:
            return 0

    @staticmethod
    @pass_context
    def get_osos_cfg(context):
        osos_cfg = {}
        for name, value in context["os_ui"].items():
            if "DefinitionRef" in value and "/EcucDefs/Os/OsOS" in value["DefinitionRef"]:
                osos_cfg.update({"ScalabilityClass": value.get("OsScalabilityClass","").get("Value","")})
                break
        return osos_cfg

    @staticmethod
    @pass_context
    def get_core_mpu_cfg(context, coreid, region_cfg):
        core_mpu_cfg = {}
        target = Function.get_target(context)
        core_mpu_cfg.update({"data_region":{}})
        core_mpu_cfg.update({"code_region":{}})
        for region_name, region_value in region_cfg.items():
            if '' == region_value["MemoryRegionOwner"] and "Core"+ str(coreid) in region_value["MemoryRegionProtectionUnit"]:
                if target not in ("AURIX2G_TC397") or '_R' in region_value["MemoryRegionAccessRights"]:
                    core_mpu_cfg["data_region"].update({region_name:region_value})
                if target in ("AURIX2G_TC397") and ('_X' in region_value["MemoryRegionAccessRights"] or '_RX' in region_value["MemoryRegionAccessRights"]):
                    core_mpu_cfg["code_region"].update({region_name:region_value})
        core_mpu_cfg["data_region"].update({"data_region_cnt":(len(core_mpu_cfg["data_region"]) + 1)})
        core_mpu_cfg["data_region"].update({"data_region_start":0})
        core_mpu_cfg["data_region"].update({"data_region_end":core_mpu_cfg["data_region"]["data_region_cnt"]})
        core_mpu_cfg["code_region"].update({"code_region_cnt":len(core_mpu_cfg["code_region"])})
        core_mpu_cfg["code_region"].update({"code_region_start":0})
        core_mpu_cfg["code_region"].update({"code_region_end":core_mpu_cfg["code_region"]["code_region_cnt"]})
        return core_mpu_cfg

    @staticmethod
    @pass_context
    def get_app_mpu_cfg(context, coreid, region_cfg, core_mpu_cfg):
        app_cfg = Function.get_app_cfg(context)
        set0_mpu_cfg = {}
        target = Function.get_target(context)
        set0_mpu_cfg.update({"data_region":{}})
        set0_mpu_cfg.update({"code_region":{}})
        app_mpu_cfg = {}
        for app_name, app_value in app_cfg.items():
            if str(coreid) == app_value["CoreNumber"]:
                app_mpu_cfg.update({app_name:{}})
                app_mpu_cfg[app_name].update({"data_region":{}})
                app_mpu_cfg[app_name].update({"code_region":{}})
                for region_name, region_value in region_cfg.items():
                    if app_name == region_value["MemoryRegionOwner"] and "Core"+ str(coreid) in region_value["MemoryRegionProtectionUnit"]:
                        if target not in ("AURIX2G_TC397") or '_R' in region_value["MemoryRegionAccessRights"]:
                            if 0 == app_value["AppMemoryProtectionIdentifier"]:
                                set0_mpu_cfg["data_region"].update({region_name:region_value})
                            else:
                                app_mpu_cfg[app_name]["data_region"].update({region_name:region_value})
                        if target in ("AURIX2G_TC397") and ('_X' in region_value["MemoryRegionAccessRights"] or '_RX' in region_value["MemoryRegionAccessRights"]):
                            if 0 == app_value["AppMemoryProtectionIdentifier"]:
                                set0_mpu_cfg["code_region"].update({region_name:region_value})
                            else:
                                app_mpu_cfg[app_name]["code_region"].update({region_name:region_value})
        set0_mpu_cfg["data_region"].update({"data_region_cnt":len(set0_mpu_cfg["data_region"])})
        set0_mpu_cfg["data_region"].update({"data_region_start":core_mpu_cfg["data_region"]["data_region_end"]})
        set0_mpu_cfg["data_region"].update({"data_region_end":(core_mpu_cfg["data_region"]["data_region_end"] + set0_mpu_cfg["data_region"]["data_region_cnt"])})
        set0_mpu_cfg["code_region"].update({"code_region_cnt":len(set0_mpu_cfg["code_region"])})
        set0_mpu_cfg["code_region"].update({"code_region_start":core_mpu_cfg["code_region"]["code_region_end"]})
        set0_mpu_cfg["code_region"].update({"code_region_end":(core_mpu_cfg["code_region"]["code_region_end"] + set0_mpu_cfg["code_region"]["code_region_cnt"])})
        for app_name, app_value in app_cfg.items():
            if str(coreid) == app_value["CoreNumber"]:
                app_mpu_cfg[app_name]["data_region"].update({"data_region_cnt":len(app_mpu_cfg[app_name]["data_region"])})
                app_mpu_cfg[app_name]["data_region"].update({"data_region_start":set0_mpu_cfg["data_region"]["data_region_end"]})
                app_mpu_cfg[app_name]["data_region"].update({"data_region_end":(set0_mpu_cfg["data_region"]["data_region_end"] + app_mpu_cfg[app_name]["data_region"]["data_region_cnt"])})
                app_mpu_cfg[app_name]["code_region"].update({"code_region_cnt":len(app_mpu_cfg[app_name]["code_region"])})
                app_mpu_cfg[app_name]["code_region"].update({"code_region_start":set0_mpu_cfg["code_region"]["code_region_end"]})
                app_mpu_cfg[app_name]["code_region"].update({"code_region_end":(set0_mpu_cfg["code_region"]["code_region_end"] + app_mpu_cfg[app_name]["code_region"]["code_region_cnt"])})
        return set0_mpu_cfg, app_mpu_cfg

    @staticmethod
    @pass_context
    def get_task_mpu_cfg(context, coreid, region_cfg, set0_mpu_cfg, app_mpu_cfg):
        app_cfg = Function.get_app_cfg(context)
        task_mpu_cfg = {}
        target = Function.get_target(context)
        for app_name, app_value in app_cfg.items():
            if str(coreid) == app_value["CoreNumber"]:
                for task_name in app_value["AppTaskList"]:
                    task_mpu_cfg.update({task_name:{}})
                    task_mpu_cfg[task_name].update({"data_region":{}})
                    task_mpu_cfg[task_name].update({"code_region":{}})
                    for region_name, region_value in region_cfg.items():
                        if task_name == region_value["MemoryRegionOwner"] and "Core"+ str(coreid) in region_value["MemoryRegionProtectionUnit"]:
                            if target not in ("AURIX2G_TC397") or '_R' in region_value["MemoryRegionAccessRights"]:
                                task_mpu_cfg[task_name]["data_region"].update({region_name:region_value})
                            if target in ("AURIX2G_TC397") and ('_X' in region_value["MemoryRegionAccessRights"] or '_RX' in region_value["MemoryRegionAccessRights"]):
                                task_mpu_cfg[task_name]["code_region"].update({region_name:region_value})
                    task_mpu_cfg[task_name]["data_region"].update({"data_region_cnt":len(task_mpu_cfg[task_name]["data_region"])})
                    task_mpu_cfg[task_name]["data_region"].update({"data_region_start":app_mpu_cfg[app_name]["data_region"]["data_region_end"]})
                    task_mpu_cfg[task_name]["data_region"].update({"data_region_end":(app_mpu_cfg[app_name]["data_region"]["data_region_end"] + task_mpu_cfg[task_name]["data_region"]["data_region_cnt"])})
                    task_mpu_cfg[task_name]["code_region"].update({"code_region_cnt":len(task_mpu_cfg[task_name]["code_region"])})
                    task_mpu_cfg[task_name]["code_region"].update({"code_region_start":app_mpu_cfg[app_name]["code_region"]["code_region_end"]})
                    task_mpu_cfg[task_name]["code_region"].update({"code_region_end":(app_mpu_cfg[app_name]["code_region"]["code_region_end"] + task_mpu_cfg[task_name]["code_region"]["code_region_cnt"])})
        task_name = "Os_PreStart_Task_Core" + str(coreid)
        task_mpu_cfg.update({task_name:{}})
        task_mpu_cfg[task_name].update({"data_region":{}})
        task_mpu_cfg[task_name].update({"code_region":{}})
        for region_name, region_value in region_cfg.items():
            if task_name == region_value["MemoryRegionOwner"] and "Core"+ str(coreid) in region_value["MemoryRegionProtectionUnit"]:
                if target not in ("AURIX2G_TC397") or '_R' in region_value["MemoryRegionAccessRights"]:
                    task_mpu_cfg[task_name]["data_region"].update({region_name:region_value})
                if target in ("AURIX2G_TC397") and ('_X' in region_value["MemoryRegionAccessRights"] or '_RX' in region_value["MemoryRegionAccessRights"]):
                    task_mpu_cfg[task_name]["code_region"].update({region_name:region_value})
        task_mpu_cfg[task_name]["data_region"].update({"data_region_cnt":len(task_mpu_cfg[task_name]["data_region"])})
        task_mpu_cfg[task_name]["data_region"].update({"data_region_start":set0_mpu_cfg["data_region"]["data_region_end"]})
        task_mpu_cfg[task_name]["data_region"].update({"data_region_end":(set0_mpu_cfg["data_region"]["data_region_end"] + task_mpu_cfg[task_name]["data_region"]["data_region_cnt"])})
        task_mpu_cfg[task_name]["code_region"].update({"code_region_cnt":len(task_mpu_cfg[task_name]["code_region"])})
        task_mpu_cfg[task_name]["code_region"].update({"code_region_start":set0_mpu_cfg["code_region"]["code_region_end"]})
        task_mpu_cfg[task_name]["code_region"].update({"code_region_end":(set0_mpu_cfg["code_region"]["code_region_end"] + task_mpu_cfg[task_name]["code_region"]["code_region_cnt"])})
        return task_mpu_cfg

    @staticmethod
    @pass_context
    def get_isr_mpu_cfg(context,coreid, region_cfg, app_mpu_cfg):
        app_cfg = Function.get_app_cfg(context)
        isr_mpu_cfg = {}
        target = Function.get_target(context)
        for app_name, app_value in app_cfg.items():
            if str(coreid) == app_value["CoreNumber"]:
                for isr_name in app_value["AppIsrList"]:
                    isr_mpu_cfg.update({isr_name:{}})
                    isr_mpu_cfg[isr_name].update({"data_region":{}})
                    isr_mpu_cfg[isr_name].update({"code_region":{}})
                    for region_name, region_value in region_cfg.items():
                        if isr_name == region_value["MemoryRegionOwner"] and "Core"+ str(coreid) in region_value["MemoryRegionProtectionUnit"]:
                            if target not in ("AURIX2G_TC397") or '_R' in region_value["MemoryRegionAccessRights"]:
                                isr_mpu_cfg[isr_name]["data_region"].update({region_name:region_value})
                            if target in ("AURIX2G_TC397") and ('_X' in region_value["MemoryRegionAccessRights"] or '_RX' in region_value["MemoryRegionAccessRights"]):
                                isr_mpu_cfg[isr_name]["code_region"].update({region_name:region_value})
                    isr_mpu_cfg[isr_name]["data_region"].update({"data_region_cnt":len(isr_mpu_cfg[isr_name]["data_region"])})
                    isr_mpu_cfg[isr_name]["data_region"].update({"data_region_start":app_mpu_cfg[app_name]["data_region"]["data_region_end"]})
                    isr_mpu_cfg[isr_name]["data_region"].update({"data_region_end":(app_mpu_cfg[app_name]["data_region"]["data_region_end"] + isr_mpu_cfg[isr_name]["data_region"]["data_region_cnt"])})
                    isr_mpu_cfg[isr_name]["code_region"].update({"code_region_cnt":len(isr_mpu_cfg[isr_name]["code_region"])})
                    isr_mpu_cfg[isr_name]["code_region"].update({"code_region_start":app_mpu_cfg[app_name]["code_region"]["code_region_end"]})
                    isr_mpu_cfg[isr_name]["code_region"].update({"code_region_end":(app_mpu_cfg[app_name]["code_region"]["code_region_end"] + isr_mpu_cfg[isr_name]["code_region"]["code_region_cnt"])})
        return isr_mpu_cfg

    @staticmethod
    @pass_context
    def get_mpu_cfg(context):
        region_list = Function.get_os_module_list(context, "/Os/OsMemoryProtection")
        if region_list:
            region_list	 = region_list["OsMemoryProtection"]
        osos_cfg = Function.get_osos_cfg(context)
        region_cfg = {}
        mpu_cfg = {}
        extern_addr_list = list()
        if osos_cfg.get("ScalabilityClass","") in ("SC3","SC4"):
            for region_name, region_value in region_list.items():
                if "DefinitionRef" == region_name:
                    continue
                region_cfg.update({region_name:{}})
                addr_str = region_value.get("OsMemoryRegionStartAddress",{}).get("Value","")
                if addr_str.startswith("0x") or addr_str.startswith("0X"):
                    region_cfg[region_name]["MemoryRegionStartAddress"] = "(uint32)" + addr_str + "uL"
                else:
                    region_cfg[region_name]["MemoryRegionStartAddress"] = "(uint32)&" + addr_str
                    extern_addr_list.append("extern uint8 " + addr_str + "[]")
                addr_str = region_value.get("OsMemoryRegionEndAddress",{}).get("Value","")
                if addr_str.startswith("0x") or addr_str.startswith("0X"):
                    region_cfg[region_name]["MemoryRegionEndAddress"] = "(uint32)" + addr_str + "uL"
                else:
                    region_cfg[region_name]["MemoryRegionEndAddress"] = "(uint32)&" + addr_str
                    extern_addr_list.append("extern uint8 " + addr_str + "[]")
                region_cfg[region_name]["MemoryRegionIndex"] = region_value.get("OsMemoryRegionIndex",{}).get("Value","")
                region_cfg[region_name]["MemoryRegionOwner"] = utils.get_last_item(region_value.get("OsMemoryRegionOwner",{}).get("ValueRef",""))
                region_cfg[region_name]["MemoryRegionProtectionUnit"] = utils.get_last_item(region_value.get("OsMemoryRegionProtectionUnit",{}).get("ValueRef",""))
                region_cfg[region_name]["MemoryRegionAccessRights"] = utils.get_last_item(region_value.get("OsMemoryRegionAccessRights",{}).get("ValueRef",""))
                region_cfg[region_name]["MemoryRegionFlags"] = utils.get_last_item(region_value.get("OsMemoryRegionFlags",{}).get("ValueRef",""))

            core_cnt = Function.get_core_cnt(context)
            for coreid in range(core_cnt):
                mpu_cfg.update({coreid:{}})
                core_mpu_cfg = Function.get_core_mpu_cfg(context, coreid, region_cfg)
                set0_mpu_cfg, app_mpu_cfg = Function.get_app_mpu_cfg(context, coreid, region_cfg, core_mpu_cfg)
                task_mpu_cfg = Function.get_task_mpu_cfg(context, coreid, region_cfg, set0_mpu_cfg, app_mpu_cfg)
                isr_mpu_cfg = Function.get_isr_mpu_cfg(context, coreid, region_cfg, app_mpu_cfg)
                mpu_cfg[coreid]["core_mpu_cfg"] = core_mpu_cfg
                mpu_cfg[coreid]["set0_mpu_cfg"] = set0_mpu_cfg
                mpu_cfg[coreid]["app_mpu_cfg"] = app_mpu_cfg
                mpu_cfg[coreid]["task_mpu_cfg"] = task_mpu_cfg
                mpu_cfg[coreid]["isr_mpu_cfg"] = isr_mpu_cfg
            mpu_cfg["extern_addr_list"] = sorted(list(set(extern_addr_list)))

        return mpu_cfg

    @staticmethod
    @pass_context
    def get_app_name_list_by_core(context, coreid):
        ret = []
        app_list = Function.get_app_cfg(context)
        for app_name, app_cfg in app_list.items():
            if int(app_cfg['CoreNumber']) == coreid:
                ret.append(app_name)
        return ret

    @staticmethod
    @pass_context
    def get_coreid_by_app_name(context, app_name):
        """
        通过应用名称获取对应的核号
        """
        app_cfg = Function.get_app_cfg(context)
        if app_name in app_cfg:
            return app_cfg[app_name]['CoreNumber']
        return ''

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_os_hook_cfg(context):
        OsOS_cfg = Function.get_os_module_list(context, "/Os/OsOS")
        os_hooks = {}
        os_hooks = OsOS_cfg.get("OsOS",{}).get("OsHooks",{})
        os_hooks_cfg = {
            'os_errorhook': {'enabled': False},
            'os_protectionhook': {'enabled': False},
            'os_shutdownhook': {'enabled': False},
            'os_startuphook': {'enabled': False}
        }
        os_hook_cnt = 0

        if os_hooks:
            os_hooks_cfg['os_errorhook'].update({
                'enabled': os_hooks.get("OsErrorHook",{}).get("Value",""),
                'stack_size': os_hooks.get("OsErrorHookStackSize",{}).get("Value",""),
                'nesting_mask': 'OS_ERRORHOOK_HOOKNESTINGMASK'
            })

            os_hooks_cfg['os_protectionhook'].update({
                'enabled': os_hooks.get("OsProtectionHook",{}).get("Value",""),
                'stack_size': os_hooks.get("OsProtectionHookStackSize",{}).get("Value",""),
                'nesting_mask': 'OS_PROTECTIONHOOK_HOOKNESTINGMASK'
            })

            os_hooks_cfg['os_shutdownhook'].update({
                'enabled': os_hooks.get("OsShutdownHook",{}).get("Value",""),
                'stack_size': os_hooks.get("OsShutdownHookStackSize",{}).get("Value",""),
                'nesting_mask': 'OS_SHUTDOWNHOOK_HOOKNESTINGMASK'
            })

            os_hooks_cfg['os_startuphook'].update({
                'enabled': os_hooks.get("OsStartupHook",{}).get("Value",""),
                'stack_size': os_hooks.get("OsStartupHookStackSize",{}).get("Value",""),
                'nesting_mask': 'OS_STARTUPHOOK_HOOKNESTINGMASK'
            })

            for cfg in os_hooks_cfg.values():
                if cfg['enabled']:
                    os_hook_cnt += 1

        return os_hooks_cfg, os_hook_cnt

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_app_hook_cfg(context):
        all_apps_hook_cfg = {}
        app_cfg = Function.get_app_cfg(context)
        os_hook_cfg, _ = Function.get_os_hook_cfg(context)
        app_hook_key_map = {"AppErrorHook": "errorhook",
                            "AppStartupHook": "startuphook",
                            "AppShutdownHook": "shutdownhook"}

        for app_name, app_val in app_cfg.items():
            app_hook_cfg = {}
            if app_val['AppHookCnt'] > 0:
                for attr_key, hook_type in app_hook_key_map.items():
                    if app_val[attr_key] != True:
                        continue
                    os_hook_name = (f'os_{hook_type}').lower()
                    if os_hook_cfg[os_hook_name]['enabled'] != True:
                        continue

                    app_hook_cfg[hook_type] = {
                        'app_hook_name': (f'{app_name}_{hook_type}').lower(),
                        'hook_type': hook_type,
                        'hook_id': (f'OS_CORE{app_val["CoreNumber"]}_{app_name}_{hook_type}_ID').upper(),
                        'hook_name': os_hook_name,
                        'stack_size': os_hook_cfg[os_hook_name]['stack_size'],
                        'stack_name': (f'OS_CORE{app_val["CoreNumber"]}_{app_name}_{hook_type}_STACK_SECTION').upper(),
                        'nesting_mask': os_hook_cfg[os_hook_name]['nesting_mask']
                    }
            all_apps_hook_cfg[app_name] = app_hook_cfg

        return all_apps_hook_cfg

    @staticmethod
    @pass_context
    def get_hook_callout_code(context):
        user_code_extract = UserCodeExtract()
        hook_callout_path = utils.get_dict_path(context, ['sys_cfg', 'path', 'bswcallout_path'])
        bsw_callout_code = user_code_extract.bsw_callout_user_code(hook_callout_path)
        return bsw_callout_code.get('hook_user_callout', {})


    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_no_stack_hook_list(context):
        ret = []
        os_cfg = Function.get_os_module_list(context, "/Os/OsOS")
        os_hook = utils.get_dict_path(os_cfg, ['OsOS', 'OsHooks'])
        if os_hook.get('OsPanicHook', {}).get('Value', '') == True:
            ret.append('os_panichook')
        if os_hook.get('OsPreTaskHook', {}).get('Value', '') == True:
            ret.append('os_pretaskhook')
        if os_hook.get('OsPostTaskHook', {}).get('Value', '') == True:
            ret.append('os_posttaskhook')
        return ret

    @staticmethod
    @pass_context
    def get_enable_hook_cfg_marcos(context):
        hook_cfg, hook_cnt = Function.get_os_hook_cfg(context)
        ret = []
        if hook_cfg['os_errorhook']['enabled'] == True:
            ret.append('CONFIG_RT_FRAMEWORK_ERROR_HOOK=y')
        if hook_cfg['os_protectionhook']['enabled'] == True:
            ret.append('CONFIG_RT_FRAMEWORK_PROTECTION_HOOK=y')
        if hook_cfg['os_shutdownhook']['enabled'] == True:
            ret.append('CONFIG_RT_FRAMEWORK_SHUTDOWN_HOOK=y')
        if hook_cfg['os_startuphook']['enabled'] == True:
            ret.append('CONFIG_RT_FRAMEWORK_STARTUP_HOOK=y')

        if hook_cnt > 0:
            ret.append('CONFIG_RT_FRAMEWORK_HOOK=y')

        no_stack_hook_list = Function.get_no_stack_hook_list(context)
        if 'os_panichook' in no_stack_hook_list:
            ret.append('CONFIG_RT_FRAMEWORK_PANIC_HOOK=y')

        if 'os_pretaskhook' in no_stack_hook_list:
            ret.append('CONFIG_RT_FRAMEWORK_PRETASK_HOOK=y')

        if 'os_posttaskhook' in no_stack_hook_list:
            ret.append('CONFIG_RT_FRAMEWORK_POSTTASK_HOOK=y')

        return ret

    @staticmethod
    @pass_context
    def get_int_level_cfg_marcos(context):
        ret = []

        int_level_cfg = utils.get_dict_path(context, ['sys_cfg', 'sys_target_dict', 'validate', 'ignore_option', 'Os'])
        ret += [
            f'CONFIG_RT_FRAMEWORK_MAX_INT_LEVEL={int_level_cfg["max_interrupt_priority"]}',
            f'CONFIG_RT_FRAMEWORK_MIN_INT_LEVEL={int_level_cfg["min_interrupt_priority"]}',
            f'CONFIG_RT_FRAMEWORK_MAX_CAT2_INT_LEVEL={int_level_cfg["max_cat2_interrupt_priority"]}',
            f'CONFIG_RT_FRAMEWORK_MAX_CAT1_INT_LEVEL={int_level_cfg["max_cat1_interrupt_priority"]}',
            f'CONFIG_RT_FRAMEWORK_MAX_TP_INT_LEVEL={int_level_cfg["max_tp_interrupt_priority"]}',
            f'CONFIG_RT_FRAMEWORK_MAX_CAT0_INT_LEVEL={int_level_cfg["max_cat0_interrupt_priority"]}'
        ]

        isr_cfgs = Function.get_isr_cfg(context)
        isr_map = {'ISR_CAT0': 0, 'ISR_TP': 0,
                   'ISR_CAT1': 0, 'ISR_CAT2': 0}

        for isr_cfg in isr_cfgs.values():
            isr_map[isr_cfg['IsrCategory']] += 1

        if isr_map['ISR_CAT0'] > 0 or isr_map['ISR_TP'] > 0 or isr_map['ISR_CAT1'] > 0:
            ret.append('CONFIG_RT_FRAMEWORK_INT_LEVEL=y')
            ret.append('CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL=y')

        if isr_map['ISR_CAT0'] > 0 or isr_map['ISR_TP'] > 0:
            ret.append('CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL=y')

        return ret

    @staticmethod
    @pass_context
    def get_osmempool_spinlock_id_set_ui(context):
        os_mempool_ui_dict = context["config_dict"].get("bsw",{}).get("mempool",{}).get("ArPackages",{}).get("ActiveEcuC",{}).get("MemPool",{})
        os_mempool_all_cfg = get_os_mempool_all_config_dict(os_mempool_ui_dict, context["config_dict"])
        osmempool_spinlockid_list = []
        for pool_name in os_mempool_all_cfg["pool_cfg"]:
            if os_mempool_all_cfg["pool_cfg"][pool_name]["MemType"] == "OS_MEMPOOLTYPE_GLOBAL":
                osmempool_spinlockid_list.append("OS_SPINLOCK_MEMPOOL_" + pool_name + "_ID")
        return osmempool_spinlockid_list


    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_rtfw_config(context):

        rtfw_config = []

        # CONFIG_RT_FRAMEWORK_MULTIAPP
        ########################################################
        app_cfg = Function.get_app_cfg(context)
        if len(app_cfg) > 1:
            rtfw_config.append("CONFIG_RT_FRAMEWORK_MULTIAPP=y")
        ########################################################

        # CONFIG_RT_FRAMEWORK_COUNTER
        ########################################################
        counter_cfg = Function.get_counter_cfg(context)
        if counter_cfg:
            rtfw_config.append("CONFIG_RT_FRAMEWORK_COUNTER=y")
        ########################################################

        # CONFIG_RT_FRAMEWORK_MEMPOOL
        ########################################################
        # mempool is a standalone configuration module not controled in rt_framework
        ########################################################

        # CONFIG_RT_FRAMEWORK_IPICALL
        ########################################################
        core_cnt = Function.get_core_cnt(context)
        if core_cnt > 1:
            rtfw_config.append("CONFIG_RT_FRAMEWORK_IPICALL=y")
        ########################################################

        # CONFIG_RT_FRAMEWORK_SPINLOCK
        ########################################################
        spinlock_cfg = Function.get_spinlock_cfg(context)
        osmempool_spinlockid_set = Function.get_osmempool_spinlock_id_set_ui(context)
        spinlock_is_configured = (spinlock_cfg or osmempool_spinlockid_set)

        core_cnt = Function.get_core_cnt(context)

        if (core_cnt > 1) and spinlock_is_configured:
            rtfw_config.append("CONFIG_RT_FRAMEWORK_SPINLOCK=y")
        ########################################################

        # CONFIG_RT_FRAMEWORK_RESOURCE
        ########################################################
        resource_cfg = Function.get_resource_cfg(context)
        if resource_cfg:
            rtfw_config.append("CONFIG_RT_FRAMEWORK_RESOURCE=y")
        ########################################################

        # CONFIG_RT_FRAMEWORK_TIMER
        ########################################################
        # rtfw timer is determined by board， so it is set in board/config/defconfig
        ########################################################

        # CONFIG_RT_FRAMEWORK_SCHTBL
        ########################################################
        schedtable_cfg = Function.get_schedtable_cfg(context)
        if schedtable_cfg:
            rtfw_config.append("CONFIG_RT_FRAMEWORK_SCHTBL=y")
        ########################################################

        # CONFIG_RT_FRAMEWORK_HOOK
        ########################################################
        rtfw_config += Function.get_enable_hook_cfg_marcos(context)
        ########################################################

        # CONFIG_RT_FRAMEWORK_SYSCALL
        ########################################################
        # syscall is determined by the board， so it is set in board/config/defconfig
        ########################################################

        # CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION
        ########################################################
        osos_cfg = Function.get_osos_cfg(context)
        if osos_cfg.get("ScalabilityClass","") in ("SC3","SC4"):
            rtfw_config.append("CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION=y")
        ########################################################

        # CONFIG_RT_FRAMEWORK_INT_LEVEL
        ########################################################
        rtfw_config += Function.get_int_level_cfg_marcos(context)
        ########################################################

        # CONFIG_RT_FRAMEWORK_ISOLATE
        ########################################################
        # isolate is a standalone configuration module not controled in rt_framework
        ########################################################

        return rtfw_config