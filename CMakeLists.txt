# ##############################################################################
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
#
# ##############################################################################

if(CONFIG_RT_FRAMEWORK)
  set(CSRCS src/os.c
            src/os_task.c
            src/os_thread.c
            src/os_isr.c
            src/os_interrupt.c
            src/os_event.c
            src/os_core.c
            src/os_wrapper.c)

  if (CONFIG_RT_FRAMEWORK_MULTIAPP)
    list(APPEND CSRCS src/os_application.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_SPINLOCK)
    list(APPEND CSRCS src/os_spinlock.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_COUNTER)
    list(APPEND CSRCS src/os_counter.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_SYSCALL)
    list(APPEND CSRCS src/os_syscall.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_IPICALL)
    list(APPEND CSRCS src/os_ipicall.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_SCHTBL)
    list(APPEND CSRCS src/os_schedtable.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_RESOURCE)
    list(APPEND CSRCS src/os_resource.c)
  endif()
  if (CONFIG_RT_FRAMEWORK_MEMPOOL)
    list(APPEND CSRCS src/os_mempool.c)
  endif ()

  if (CONFIG_RT_FRAMEWORK_HOOK)
    list(APPEND CSRCS src/os_hook.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_ISOLATE)
    list(APPEND CSRCS src/os_isolate.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Isolate/isolate_ppc_cfg.c)
  endif()

  set(INCLUDE_DIRS ${NUTTX_DIR}/sched
                   ${CMAKE_CURRENT_LIST_DIR}/src
                   ${CMAKE_CURRENT_LIST_DIR}/inc
                   ${NUTTX_APPS_DIR}/components/include/autosar)

  list(APPEND INCLUDE_DIRS ${NUTTX_APPS_DIR}/${APPS_DIR}/apps)

list(APPEND INCLUDE_DIRS ${GENDATA_DIR}/Os
                         ${GENDATA_DIR}/Memmap
                         ${GENDATA_DIR}/Isolate)

if (CONFIG_RT_FRAMEWORK_MEMPOOL)
  list(APPEND INCLUDE_DIRS ${GENDATA_DIR}/MemPool)
endif ()

  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	  add_compile_options(-Werror -Wextra -pedantic -Wstrict-prototypes)
  endif()
  nuttx_add_library(rt_framework STATIC)
  target_sources(rt_framework PRIVATE ${CSRCS})
  target_include_directories(rt_framework PRIVATE ${INCLUDE_DIRS} ${VENDOR_THIRD_PARTY_DRIVER_INCDIR})
endif()

