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

from rt_framework.config_plug_in.generator import utils
from func_module.generator.common.common import FunctionBase
from jinja2 import pass_context
from decimal import Decimal


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
        os_mod_list = {}
        for name, value in context["os_ui"].items():
            if "DefinitionRef" in value and module_path in value["DefinitionRef"]:
                os_mod_list[name] = value
        return os_mod_list

    @staticmethod
    @pass_context
    def get_access_app_mask(context, app_set, index_size):
        app_mask = ""
        indent = " " * index_size
        app_set = sorted(list(set(app_set)))
        app_cfg = Function.get_app_cfg(context)
        for app in app_set:
            if app != '':
                app_mask = app_mask + '|\n' + indent + "OS_APPID2MASK(%s)" % (app_cfg[app]["AppId"])
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
    def resolve_ref_list(ref_type, cfg_dict):
        ref_list = list()
        # When only one reference is configurated,
        # the ValueRef is an instance of str instead of list.
        if isinstance(cfg_dict[ref_type]["ValueRef"], str):
            ref_list.append(utils.get_last_item(
                cfg_dict[ref_type]["ValueRef"]))
        elif isinstance(cfg_dict[ref_type]["ValueRef"], list):
            for item_dir in cfg_dict[ref_type]["ValueRef"]:
                ref_list.append(utils.get_last_item(item_dir))
        return sorted(list(set(ref_list)), key=lambda x: (len(x), x.lower()))

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_task_cfg(context):
        task_dict = Function.get_os_module_list(context, '/AUTOSAR/EcucDefs/Os/OsTask')
        task_cfg = {}

        for name, value in task_dict.items():
            task_cfg[name] = {}
            if "OsTaskAutostart" in value:
                OsTaskAutostart = utils.get_last_item(
                    value["OsTaskAutostart"]["OsTaskAppModeRef"]["ValueRef"])
                task_cfg[name]["TaskAutostart"] = "TRUE" if OsTaskAutostart == "OSAUTOSTARTMODE" else "FALSE"
            else:
                task_cfg[name]["TaskAutostart"] = "FALSE"
            task_cfg[name]["TaskType"] = value["OsTaskType"]["Value"]

            task_cfg[name]["TaskPriority"] = value["OsTaskPriority"]["Value"]
            task_cfg[name]["TaskActivation"] = value["OsTaskActivation"]["Value"]
            task_cfg[name]["TaskStackSize"] = value["OsTaskStackSize"]["Value"]

            task_cfg[name]["TaskAccessApp"] = \
                Function.resolve_ref_list("OsTaskAccessingApplication", value)
            task_cfg[name]["TaskEventRef"] = \
                Function.resolve_ref_list("OsTaskEventRef", value)

            # add OsTaskUsesFpu
            task_cfg[name]["TaskUsesFpu"] = 1 if value["OsTaskUsesFpu"]["Value"] else 0

        return task_cfg

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
        app_cfg = {}

        # solve the basic information about application config
        for name, value in app_dict.items():
            app_cfg[name] = {}
            app_coreid = value["OsApplicationCoreRef"]["ValueRef"][-1]
            app_cfg[name]["CoreNumber"] = app_coreid

            app_cfg[name]["AppTaskList"] = \
                Function.resolve_ref_list("OsAppTaskRef", value)
            app_cfg[name]["AppTaskTable"] = \
                name.lower() + "_task_cfg_ref_table"
            app_cfg[name]["AppCounterList"] = \
                Function.resolve_ref_list("OsAppCounterRef", value)
            app_cfg[name]["AppCounterTable"] = \
                name.lower() + "_counter_cfg_ref_table"
            app_cfg[name]["AppIsrList"] = \
                Function.resolve_ref_list("OsAppIsrRef", value)
            app_cfg[name]["AppIsrTable"] = \
                name.lower() + "_isr_cfg_ref_table"
            app_cfg[name]["AppScheduleTableList"] =\
                Function.resolve_ref_list("OsAppScheduleTableRef", value)
            app_cfg[name]["AppSchedTblTable"] = \
                name.lower() + "_schedtable_cfg_ref_table"

            app_cfg[name]["AppPrivileged"] = "TRUE" if value["OsIsPrivileged"]["Value"] else "FALSE"

            app_cfg[name]["app_name"] = f"OS_CORE{app_coreid}_{name}"
            app_cfg[name]["AppId"] = f"{app_cfg[name]['app_name'].upper()}_ID"
            app_cfg[name]["AppInstSection"] = f"OS_{name.upper()}_VAR_NO_INIT_SECTION"

        # get the mapping between coreid and bsw app
        core_bsw_app_dict = {}
        cores_list = Function.get_os_module_list(context, "/Os/OsCore")
        for value in cores_list.values():
            bsw_appname = utils.get_last_item(value["OsCoreBswApplicationRef"]["ValueRef"])
            core_bsw_app_dict[app_cfg[bsw_appname]["CoreNumber"]] = bsw_appname

        # find the bsw app for every applition
        for name, value in app_dict.items():
            app_cfg[name]["BswAppName"] = core_bsw_app_dict[app_cfg[name]["CoreNumber"]]

        return app_cfg

    @staticmethod
    @pass_context
    def get_isr_cfg(context):
        isr_dict = Function.get_os_module_list(context, "/Os/OsIsr")
        isr_cfg = {}

        for name, value in isr_dict.items():

            isr_cfg[name] = {}

            if value["OsIsrInitialEnableInterruptSource"]["Value"]:
                isr_cfg[name]["IsrEnableOnInit"] = "TRUE"
            else:
                isr_cfg[name]["IsrEnableOnInit"] = "FALSE"


            isr_cfg[name]["IsrIntPriority"] = value["OsIsrInterruptPriority"]["Value"]
            isr_cfg[name]["IsrIntSource"] = value["OsIsrInterruptSource"]["Value"]

            if value["OsIsrCategory"]["Value"] == "CATEGORY_2":
                isr_cfg[name]["IsrCategory"] = "ISR_CAT2"
                isr_cfg[name]["IsrContext"] = "OS_ISR2_CONTEXT_ID"
            elif value["OsIsrCategory"]["Value"] == "CATEGORY_1":
                isr_cfg[name]["IsrCategory"] = "ISR_CAT1"
                isr_cfg[name]["IsrContext"] = "OS_ISR1_CONTEXT_ID"
            elif value["OsIsrCategory"]["Value"] == "CATEGORY_TP":
                isr_cfg[name]["IsrCategory"] = "ISR_TP"
                isr_cfg[name]["IsrContext"] = "OS_TPISR_CONTEXT_ID"
            else:
                isr_cfg[name]["IsrCategory"] = "ISR_CAT0"
                isr_cfg[name]["IsrContext"] = "OS_ISR0_CONTEXT_ID"

            isr_cfg[name]["IsrEntryFunc"] = value["OsIsrEntryFunctionName"]["Value"]
            # not supported value
            isr_cfg[name]["IsrEntryParam"] = "NULL_PTR"

        return isr_cfg

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

            cfg_i["Successor"] = Function.resolve_ref_list('OsSpinlockSuccessor', spin_val)

            cfg_i["AccessingApplication"] = \
                Function.resolve_ref_list('OsSpinlockAccessingApplication', spin_val)
            spinlock_cfg.update({spin: cfg_i})

        Function.solve_spinlock_order(spinlock_cfg)

        return utils.sort_dict_by_key(spinlock_cfg)

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
                if ev_mask:
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
        elif target in ("E3650"):
            return False
        else:
            raise TypeError("没有适配的架构！")

    @staticmethod
    @pass_context
    def get_hardware_timer_channel(context, hardware_timer_name):
        for name, value in context["os_ui"].items():
            if "DefinitionRef" in value and "/Os/OsPublishedInformation" in value["DefinitionRef"]:
                publish_info = value["OsDerivativeInformation"]
                break
        return publish_info.get(hardware_timer_name,"").get("OsHardWareTimerChannel",{}).get("Value","")

    @staticmethod
    @pass_context
    def get_hardware_timer_frequence(context, hardware_timer_channel_name):
        for name, value in context["os_ui"].items():
            if "DefinitionRef" in value and "/Os/OsPublishedInformation" in value["DefinitionRef"]:
                publish_info = value["OsDerivativeInformation"]
                break
        return publish_info[hardware_timer_channel_name]["OsHardWareTimerClockFrequency"]["Value"]

    @staticmethod
    @pass_context
    def check_interruptsource_correspond_to_timer(context, isr_name, hardware_timer_channel_name):
        target = Function.get_target(context)
        isr_cfg = Function.get_isr_cfg(context)
        if target in ("AURIX2G_TC397"):
            if hardware_timer_channel_name == "STM0":
                return isr_cfg[isr_name]["IsrIntSource"] == 768
            elif hardware_timer_channel_name == "STM1":
                return isr_cfg[isr_name]["IsrIntSource"] == 776
            elif hardware_timer_channel_name == "STM2":
                return isr_cfg[isr_name]["IsrIntSource"] == 784
            elif hardware_timer_channel_name == "STM3":
                return isr_cfg[isr_name]["IsrIntSource"] == 792
            elif hardware_timer_channel_name == "STM4":
                return isr_cfg[isr_name]["IsrIntSource"] == 800
            elif hardware_timer_channel_name == "STM5":
                return isr_cfg[isr_name]["IsrIntSource"] == 808
            else:
                return False
        elif target in ("E3650"):
            return isr_cfg[isr_name]["IsrIntSource"] == 30
        else:
            raise TypeError("没有适配的架构！")

    @staticmethod
    @pass_context
    def check_isrhandler_correspond_to_timer(context, isr_name):
        target = Function.get_target(context)
        isr_cfg = Function.get_isr_cfg(context)
        return True

    @staticmethod
    @pass_context
    def check_isr_is_initialenable(context, isr_name):
        target = Function.get_target(context)
        isr_cfg = Function.get_isr_cfg(context)
        return isr_cfg[isr_name]["IsrEnableOnInit"] == "FALSE"
        return True

    @staticmethod
    @pass_context
    def get_isr_counter(context):
        target = Function.get_target(context)
        isr_counter = {}
        counter_list = Function.get_os_module_list(context, "/Os/OsCounter")
        for counter_name, counter_value in counter_list.items():
            if "OsDriver" in counter_value:
                isr_name = utils.get_last_item(counter_value["OsDriver"]["OsIsrRef"]["ValueRef"])
                if isr_name in isr_counter:
                    raise Exception(f"{counter_name} OsIsrRef is used to multiple counter !!!!!!!!\n")
                HardwareTimerName = utils.get_last_item(counter_value["OsDriver"]["OsHardwareTimerChannelRef"]["ValueRef"])
                HardwareTimerChannelName = Function.get_hardware_timer_channel(context, HardwareTimerName)
                if Function.check_interruptsource_correspond_to_timer(context, isr_name, HardwareTimerChannelName) == False:
                    raise Exception(f"{counter_name} OsIsrRef does not correspond to the OsHardwareTimerChannelRef !!!!!!!!\n")
                if Function.check_isrhandler_correspond_to_timer(context, isr_name) == False:
                    raise Exception(f"{counter_name} OsIsrRef handler is invailed !!!!!!!!\n")
                if Function.check_isr_is_initialenable(context, isr_name) == False:
                    raise Exception(f"{counter_name} OsIsrRef must not be InitialEnable !!!!!!!!\n")
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
            if CounterType == "HARDWARE" and "OsDriver" not in counter_value:
                raise Exception(f"{counter_name} is {CounterType} Counter, which must have OsDriver !!!!!!!!\n")
            if CounterType == "SOFTWARE":
                TimerType = "TIMERTYPE_SOFTWARE"
                CounterTicksPerBase = 1
                IsrRef = ""
                CounterMaxInternalTickValue = 2 * CounterMaxAllowedValue + 1
            else:
                TimerType = "TIMERTYPE_HIGH_RESOLUTION"
                if CounterMaxAllowedValue != 1073741823:
                    CounterMaxAllowedValue = 1073741823
                HardwareTimerName = utils.get_last_item(counter_value["OsDriver"]["OsHardwareTimerChannelRef"]["ValueRef"])
                HardwareTimerChannelName = Function.get_hardware_timer_channel(context, HardwareTimerName)
                HardwareTimer = float(Decimal("0.000001") / Decimal(str(Function.get_hardware_timer_frequence(context, HardwareTimerName))))
                if (SecondsPerTick > HardwareTimer + 0.00000001) or (SecondsPerTick < HardwareTimer - 0.00000001):
                    raise Exception(f"{counter_name} OsSecondsPerTick must be set to {HardwareTimer} !!!!!!!!\n")
                    SecondsPerTick = HardwareTimer
                CounterTicksPerBase = 1
                IsrRef = utils.get_last_item(counter_value["OsDriver"]["OsIsrRef"]["ValueRef"])
                CounterMaxInternalTickValue = 4 * CounterMaxAllowedValue + 3
            CounterAccessApp = Function.resolve_ref_list('OsCounterAccessingApplication', counter_value)
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
            if "OsScheduleTableSyncStrategy" in value:
                sync_strategy = value["OsScheduleTableSyncStrategy"]["Value"]
                if sync_strategy == "EXPLICIT":
                    schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "EXPLICIT"
                elif sync_strategy == "IMPLICIT":
                    schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "IMPLICIT"
                else:
                    schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "NONE"
            else:
                schedtable_cfg[name]["OsScheduleTableSync"]["OsScheduleTblSyncStrategy"] = "NONE"

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
                    Function.resolve_ref_list("OsSchTblAccessingApplication", value)
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
    def get_memmap_module_name(context):
        return context['module']

    @staticmethod
    @pass_context
    @utils.cache_once(_cache)
    def get_memmap_cfg(context):
        memmap_dict = Function.get_os_module_list(context, '/AUTOSAR/EcucDefs/Os/OsMemmoryMap')['OsMemmoryMap']
        internal_layer = ''
        layer_dict = {}
        module_dict = {}
        for key, mem_cfg in memmap_dict.items():
            if not isinstance(mem_cfg, dict):
                continue
            item_type = mem_cfg.get('DefinitionRef', '')
            if item_type.endswith('OsMemmapModule'):
                mod_dict = {
                    'use_calib': mem_cfg['OsMemmapUseCalibration']['Value'],
                    'apps': Function.resolve_ref_list('OsMemmapApplicationRef', mem_cfg),
                    'layer': utils.get_last_item(mem_cfg['OsMemmapUseLayer']['ValueRef'])
                }
                module_dict[key.lower()] = mod_dict

            elif item_type.endswith('OsMemmapLayer'):
                layer = {
                    'internal': mem_cfg['OsInternalLayer']['Value']
                }
                if layer['internal']:
                    internal_layer = key
                layer_dict[key.lower()] = layer
        memmap_cfg = {
            "layers": layer_dict,
            "modules": module_dict,
            "intenral_layer": internal_layer
        }

        return memmap_cfg

    @staticmethod
    def __format_map_info(key, val):
        return {
            'map_object': key,
            'map_target': val
        }

    @staticmethod
    def _gen_flash_map_info(layer, apps):
        if len(apps) != 1:
            app = 'BSW_APPS'
        else:
            app = apps[0]
        ret = [
            Function.__format_map_info("CALLOUT_CODE", f'{app}_CODE_{layer}_TEXT'),
            Function.__format_map_info("CODE", f'{app}_CODE_{layer}_TEXT'),
            Function.__format_map_info("CONST", f'{app}_CONST_{layer}_RODATA')
        ]

        return ret

    @staticmethod
    def _gen_calib_map_info(layer, calib_app):
        ret = [
            Function.__format_map_info("CALIB_CONST", f'{calib_app}_CALIBRATION_{layer}_RODATA')
        ]
        return ret

    @staticmethod
    def __common_gen_apps_map_info(scope_map, apps, layer):
        data_types = ['DATA', 'BSS']
        ret = []
        if isinstance(apps, str):
            for scope_key, scope_val in scope_map.items():
                for d_type in data_types:
                    ret.append(Function.__format_map_info(f'{scope_key}_{d_type}',
                                                          f'{apps}_{scope_val}_{layer}_{d_type}'))
        elif isinstance(apps, list):
            for app in apps:
                for scope_key, scope_val in scope_map.items():
                    for d_type in data_types:
                        ret.append(Function.__format_map_info(f'{app}_{scope_key}_{d_type}',
                                                              f'{app}_{scope_val}_{layer}_{d_type}'))
        return ret

    @staticmethod
    def _gen_no_app_map_info(layer):
        var_scope_property_map = {
            "PRIVATE":"MULTI_APP_SHARE"
        }
        app = 'BSW_APPS'
        ret = Function.__common_gen_apps_map_info(var_scope_property_map,
                                                  app, layer)

        return ret


    @staticmethod
    def _gen_single_app_map_info(layer, apps):
        var_scope_property_map = {
            "PRIVATE":"PRIVATE",
            "SEMISHARED":"SEMISHARED"
        }
        return Function.__common_gen_apps_map_info(var_scope_property_map,
                                                  apps[0], layer)

    @staticmethod
    def _gen_multi_app_map_info(layer, apps):
        var_scope_property_map = {
            "PRIVATE":"PRIVATE",
            "SEMISHARED":"SEMISHARED"
        }
        return Function.__common_gen_apps_map_info(var_scope_property_map,
                                                  apps, layer)

    @staticmethod
    @pass_context
    def get_memmap_info(context, module):
        memmap_cfg = Function.get_memmap_cfg(context)
        mod_cfg = memmap_cfg['modules'][module]
        ref_apps = mod_cfg['apps']
        ref_layer = mod_cfg['layer']
        memmap_info = []

        memmap_info += Function._gen_flash_map_info(ref_layer, ref_apps)
        if mod_cfg['use_calib']:
            # We now use the bsw app of the master core as the
            # default calibration app as a temporary strategy.
            default_calib_app = Function.get_bsw_app_from_coreid(context, coreid=0)
            memmap_info += Function._gen_calib_map_info(ref_layer, default_calib_app)

        if len(ref_apps) == 0:
            memmap_info += Function._gen_no_app_map_info(ref_layer)
        elif len(ref_apps) == 1:
            memmap_info += Function._gen_single_app_map_info(ref_layer, ref_apps)
        else:
            memmap_info += Function._gen_multi_app_map_info(ref_layer, ref_apps)

        ret_info = {'mapping_info': memmap_info}

        return ret_info

    @staticmethod
    def __gen_memmap_internal_info(apps, attr_pairs, layers):
        ret = {}
        for app in apps:
            for attr in attr_pairs:
                for layer in layers:
                    map_key = (f'{app}_{attr[0]}_{layer}_{attr[1]}').upper()
                    map_val = (f'.{attr[1]}.{attr[0]}.{app}.{layer}').lower()
                    ret[map_key] = map_val
        return ret

    @staticmethod
    @pass_context
    def get_memmap_internal_cfg(context):
        ret = {}
        memmap_cfg = Function.get_memmap_cfg(context)
        app_cfg = Function.get_app_cfg(context)

        int_layer = memmap_cfg['intenral_layer']
        all_layer = memmap_cfg['layers']
        ret.update({
            'CODE': f'.text.code.all_apps.{int_layer}',
            'CALLOUT_CODE': f'.text.code.all_apps.{int_layer}',
            'CONST': f'.rodata.const.all_apps.{int_layer}'
        })

        apps = ['all_apps', 'bsw_apps']
        core_num = Function.get_core_cnt(context)
        apps += [f'core{x}_apps' for x in range(core_num)]

        attr_pairs = [('multi_app_share', 'data'), ('multi_app_share', 'bss'),
                      ('code', 'text'), ('const', 'rodata')]
        ret.update(Function.__gen_memmap_internal_info(apps, attr_pairs, all_layer))

        # per application
        attr_pairs = [('semishared', 'data'), ('private', 'data'),
                      ('semishared', 'bss'), ('private', 'bss'),
                      ('calibration', 'rodata'), ('const', 'rodata'),
                      ('code', 'text')]
        ret.update(Function.__gen_memmap_internal_info(app_cfg.keys(), attr_pairs, all_layer))

        return ret
