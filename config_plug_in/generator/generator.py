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

import os

from func_module.health_monitor.logger import logger
from jinja2 import Environment, FileSystemLoader, StrictUndefined
from rt_framework.config_plug_in.generator.utils import get_last_item, get_dict_path

import rt_framework.config_plug_in.generator.context as context

CUR_FILE_PATH = os.path.dirname(__file__)
DEFAULT_TEMPLATE_FILE_DIR = os.path.join(CUR_FILE_PATH, "template")

def Os_CoreInfo_Get(ecuc_dict: dict):
    ret = {}

    for k, v in ecuc_dict["EcucHardware"].items():
        if "EcucCoreDefinition" not in k:
            continue
        coreid = str(v['EcucCoreId']['Value'])
        hw_coreid = str(get_last_item(v['EcucCoreHwRef']['ValueRef'])[-1])
        ret['Os_Core' + coreid] = {
            'core_id': 'OS_CORE_ID_' + coreid,
            'PhyId': 'OS_CORE_PHYSICAL_ID_' + hw_coreid
        }

    return ret

def render_os_template(template, out_dir, file_name, config):
    if template is not None:
        file_path = os.path.join(out_dir, file_name)
        with open(file_path, "w", encoding="utf-8") as out_stream:
            out_stream.write(template.render(config))
            out_stream.flush()
        logger.info("Finish generating %s file" % (file_name))

def generate_memmap_modulue_header(template, out_dir, config):
    memmap_modules = config['os_ui'].get('OsMemmoryMap', {})
    for item_name, item in memmap_modules.items():
        if not isinstance(item, dict):
            continue
        item_type = item.get('DefinitionRef', '')
        if item_type.endswith('OsMemmapModule'):
            mod_name = item_name.lower()
            gen_file_name = f'{mod_name}_memmap.h'
            module_config = {**config, 'module': mod_name}
            render_os_template(template, out_dir, gen_file_name, module_config)

def generate_memmap(env_ui, config):
    memmap_output_dir = os.path.realpath(os.path.join(config['sys_cfg']['gendata_path'], "Memmap"))
    if not os.path.exists(memmap_output_dir):
        os.makedirs(memmap_output_dir)
    template = env_ui.get_template("module_memmap.h.jinja2")
    generate_memmap_modulue_header(template, memmap_output_dir, config)

    template = env_ui.get_template("memmap.h.jinja2")
    render_os_template(template, memmap_output_dir, "memmap.h", config)

    template = env_ui.get_template("memmap_int.h.jinja2")
    render_os_template(template, memmap_output_dir, "memmap_int.h", config)


def generate_dynamic_code(sys_config: dict, config_dict: dict):
    context.Function.clear_caches()

    # get EcuC Configs
    ecuc_dict = get_dict_path(config_dict, ['bsw', 'ecuc', 'ArPackages', 'ActiveEcuC', 'EcuC'])

    # get core information and hw coreid information from UI input and add to os dict
    core_dict = Os_CoreInfo_Get(ecuc_dict)

    project_target = sys_config.get("target", "")
    if project_target in sys_config["sys_target_dict"]["validate"]["ignore_option"]:
        sys_config["sys_target_dict"]["validate"]["ignore_option"] = \
            sys_config["sys_target_dict"]["validate"]["ignore_option"][project_target]

    output_dir = os.path.realpath(
        os.path.join(sys_config["gendata_path"], "Os"))
    # more hardware platform template on the way~

    # save the configuration of ui
    input_ui_dict = get_dict_path(config_dict, ['bsw', 'os', 'ArPackages', 'ActiveEcuC', 'Os'])

    # Build our rendering environment.
    env_ui = Environment(loader=FileSystemLoader(
        os.path.abspath(os.path.join(DEFAULT_TEMPLATE_FILE_DIR, "ui"))),
                         undefined=StrictUndefined)
    env_ui.filters.update(context.get_filters())
    env_ui.globals.update(context.get_functions())

    config = {
        "os_ui": input_ui_dict,
        "sys_cfg": sys_config,
        "os_core": core_dict
    }

    # ################################################## #
    #    TASK                                            #
    # ################################################## #
    template = env_ui.get_template("task_cfg.h.jinja2")
    render_os_template(template, output_dir, "task_cfg.h", config)

    template = env_ui.get_template("task_cfg.c.jinja2")
    render_os_template(template, output_dir, "task_cfg.c", config)

    # ################################################## #
    #    STACK                                           #
    # ################################################## #
    template = env_ui.get_template("stack_cfg.h.jinja2")
    render_os_template(template, output_dir, "stack_cfg.h", config)

    # ################################################## #
    #    APPLICATION                                     #
    # ################################################## #
    template = env_ui.get_template("application_cfg.h.jinja2")
    render_os_template(template, output_dir, "application_cfg.h", config)

    template = env_ui.get_template("application_cfg.c.jinja2")
    render_os_template(template, output_dir, "application_cfg.c", config)

    # ################################################## #
    #    CORE                                            #
    # ################################################## #
    template = env_ui.get_template("core_cfg.c.jinja2")
    render_os_template(template, output_dir, "core_cfg.c", config)

    template = env_ui.get_template("core_cfg.h.jinja2")
    render_os_template(template, output_dir, "core_cfg.h", config)

    # ################################################## #
    #    IpiCall                                         #
    # ################################################## #
    template = env_ui.get_template("ipicall_cfg.c.jinja2")
    render_os_template(template, output_dir, "ipicall_cfg.c", config)

    template = env_ui.get_template("ipicall_cfg.h.jinja2")
    render_os_template(template, output_dir, "ipicall_cfg.h", config)

    # ################################################## #
    #    ISR                                             #
    # ################################################## #
    template = env_ui.get_template("isr_cfg.c.jinja2")
    render_os_template(template, output_dir, "isr_cfg.c", config)

    template = env_ui.get_template("isr_cfg.h.jinja2")
    render_os_template(template, output_dir, "isr_cfg.h", config)

    # ################################################## #
    #    SPINLOCK                                        #
    # ################################################## #
    template = env_ui.get_template("spinlock_cfg.h.jinja2")
    render_os_template(template, output_dir, "spinlock_cfg.h", config)

    template = env_ui.get_template("spinlock_cfg.c.jinja2")
    render_os_template(template, output_dir, "spinlock_cfg.c", config)

    # ################################################## #
    #    EVENT                                           #
    # ################################################## #
    template = env_ui.get_template("event_cfg.h.jinja2")
    render_os_template(template, output_dir, "event_cfg.h", config)

    # ################################################## #
    #    SCHEDTABLE                                      #
    # ################################################## #
    template = env_ui.get_template("schedtable_cfg.h.jinja2")
    render_os_template(template, output_dir, "schedtable_cfg.h", config)

    template = env_ui.get_template("schedtable_cfg.c.jinja2")
    render_os_template(template, output_dir, "schedtable_cfg.c", config)

    # ################################################## #
    #    Compile                                         #
    # ################################################## #
    template = env_ui.get_template("os_compiler_cfg.h.jinja2")
    render_os_template(template, output_dir, "os_compiler_cfg.h", config)

    # ################################################## #
    #    COUNTER                                         #
    # ################################################## #
    template = env_ui.get_template("counter_cfg.h.jinja2")
    render_os_template(template, output_dir, "counter_cfg.h", config)

    template = env_ui.get_template("counter_cfg.c.jinja2")
    render_os_template(template, output_dir, "counter_cfg.c", config)

    # ################################################## #
    #    MemMap                                          #
    # ################################################## #
    generate_memmap(env_ui, config)


    return True
