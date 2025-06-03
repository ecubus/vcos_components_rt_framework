# This Python file uses the following encoding: utf-8
############################################################################
# Copyright (c) 2025 Li Auto Inc. and its affiliates
# Licensed under the Apache License, Version 2.0(the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
############################################################################
from func_module.validation.common.basic_rule import BasicRule, timing_decorator
from durable.lang import *
from durable.engine import *
from .Os_ruleutils import OsRuleutils

class Os016004Rule(BasicRule):
    """调度表设置为周期执行,OsScheduleTableDuration不能超出OsCounterMaxAllowedValue"""
    ruleset_name = "Os016004"
    validation_result = False

    def __init__(self, logs, ecuc_os_dict) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = ecuc_os_dict
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "If OsScheduleTableRepeating=true, OsScheduleTableDuration must be less than OsCounter's OsCounterMaxAllowedValue"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all(+m.OsScheduleTableDuration)
                def action(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        ecuc_os_dict = self.ecuc_os_dict
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsScheduleTable获取数据
            if "/Os/OsScheduleTable" in os_instance["DefinitionRef"] and "OsScheduleTableRepeating" in os_instance:
                OsCounterMaxAllowedValue,OsScheduleTableDuration = 0,0
                OsScheduleTableRepeating = os_instance["OsScheduleTableRepeating"].get("Value",False)
                if OsScheduleTableRepeating:
                    OsScheduleTableDuration = os_instance["OsScheduleTableDuration"].get("Value",0)
                    ScheduleTable_counter = OsRuleutils().get_module_property_valueref("OsScheduleTable", os_key, "OsScheduleTableCounterRef", ecuc_os_dict)
                    if ScheduleTable_counter:
                        OsCounterMaxAllowedValue = OsRuleutils().get_counter_OsCounterMaxAllowedValue(ScheduleTable_counter[0], ecuc_os_dict)
                    validation_data = {}
                    param_path = "/ActiveEcuC/Os/" + os_key
                    validation_data.update({"OsScheduleTable": os_key})
                    validation_data.update({"OsCounterMaxAllowedValue": OsCounterMaxAllowedValue})
                    validation_data.update({"OsScheduleTableDuration": OsScheduleTableDuration})
                    try:
                        self.__class__.validation_result = False
                        if OsScheduleTableDuration < OsCounterMaxAllowedValue:
                            post(self.__class__.ruleset_name, validation_data)
                    except Exception as e:
                        pass
                    # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                    if self.__class__.validation_result:
                        self.vlog(self.validation_code, self.validation_summary,
                                f"validation_data:{validation_data}", param_path, "pass")
                    else:
                        self.vlog(self.validation_code, self.validation_summary,
                                f"validation_data:{validation_data}", param_path, "fail")
