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
from func_module.validation.common.validation_def import ValidationData, ExceptionRuleLog
import os
import json
import importlib
from typing import Dict, List, Optional

class OsRule:
    """
    OS模块组件验证规则类
    负责执行各个OS组件的验证规则检查
    """

    validate_rule_groups = {
        "OsApplication": ["001001","001003","001006","002003","003001","014007"],
        "OsResource": ["013001","013006","014012"],
        "OsCounter": ["003003","014003","016011"],
        "OsEvent": ["003004","001002","014017"],
        "OsIsr": ["011003","014004", "013008"],
        "OsIoc": ["001003","003005","003006","011001", "011002", "013011"],
        "OsScheduleTable": ["001007","002002","012003","013010","016003","014005","016004","016007"],
        "OsSpinlock": ["003002", "016001"],
        "OsTask": ["013004","014006","014008","014009","014010","014015","014016","014018","014020","014021","014022","014023"],
        "OsMemoryProtection": ["016008","016009","016010", "016012"],
    }

    def __init__(self) -> None:
        """初始化验证日志字典"""
        self.logs: Dict = {}
        self._validation_results: Dict = {}
        self._validation_errors: Dict = {}
        self._component_logs: Dict = {}

    def _validate_component_group(self, validate_marks: list, ecuc_os_dict: Dict, validation_cfg: Dict, component_name: str, system_path: str) -> None:
        """
        执行单个组件组的验证
        Args:
            validate_marks: 验证标记列表
            ecuc_os_dict: OS配置字典
            validation_cfg: 验证配置
            component_name: 组件名称(如OsScheduleTable)
            system_path: 验证执行路径
        """
        if validation_cfg.get(component_name, {}).get("skip", False):
            return

        for validate_mark in validate_marks:
            try:
                module_path = f'.Os{validate_mark}'
                validate_rule_package = importlib.import_module(module_path, __package__)
                validate_rule_class = getattr(validate_rule_package, f'Os{validate_mark}Rule')

                os_rule_obj = validate_rule_class(self.logs, ecuc_os_dict, validation_cfg, system_path)
                os_rule_obj.validation_run()

            except Exception as e:
                error_msg = f"{component_name} validation failed: {type(e).__name__}: {str(e)}"
                ExceptionRuleLog.set_rule_exception_log(
                    self.logs,
                    "Os",
                    validate_mark,
                    error_msg
                )

    def validate(self, validation_data: ValidationData) -> Dict:
        """
        执行所有OS组件的验证
        Args:
            validation_data: 验证数据对象
        Returns:
            Dict: 验证日志字典
        """
        sys_info = validation_data.sys_info_class
        system_path = sys_info.system_path
        bsw_ecuc_cfg_class_dict = validation_data.bsw_ecuc_dict
        ecuc_os_dict = bsw_ecuc_cfg_class_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        validation_cfg = (
            sys_info.system_target_info.get("validate", {})
            .get("ignore_option", {})
            .get("Os", {})
        )
        for component_name, validate_marks in self.validate_rule_groups.items():
            try:
                self._validate_component_group(validate_marks, ecuc_os_dict, validation_cfg, component_name, system_path)
            except Exception as e:
                validate_code = validate_marks[0] if validate_marks else "UNKNOWN"
                ExceptionRuleLog.set_rule_exception_log(
                    self.logs,
                    "Os",
                    validate_code,
                    f"{component_name} group validation failed: {type(e).__name__}: {str(e)}"
                )
        output_path = os.path.join(os.path.dirname(__file__), 'ecuc_os_dict.json')
        try:
            with open(output_path, 'w', encoding='utf-8') as f:
                json.dump(ecuc_os_dict, f, indent=4, ensure_ascii=False)
        except Exception as e:
            ExceptionRuleLog.set_rule_exception_log(
                self.logs,
                "Os",
                "SAVE_JSON",
                f"Failed to save validation results: {type(e).__name__}: {str(e)}"
            )
        return self.logs

    def get_logs(self) -> Dict:
        """
        获取验证日志

        Returns:
            Dict: 验证日志字典
        """
        return self.logs