# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Sets up the VST3 SDK fetched via CPM (expects vst3sdk_SOURCE_DIR).
#
# Only creates the library targets we need (base, pluginterfaces, sdk_common,
# sdk, sdk_hosting) - no examples, VSTGUI or other tooling - using the SDK's
# own cmake modules. Consumers:
#   - plugins: smtg_add_vst3plugin(<tgt> ...) + target_link_libraries(<tgt> sdk)
#   - hosts: target_link_libraries(<tgt> sdk_hosting)
#
# The include() is wrapped in a function because SMTG_Global.cmake applies
# unwanted directory-scope settings at include time (output directories,
# etc.). Platform variables (SMTG_LINUX & friends) are CACHE INTERNAL and
# remain available globally, and all smtg_* functions are global once defined.

# Options consumed by smtg_* functions - set before the include so the SMTG
# option() declarations pick them up (CMP0077)
set(SMTG_CREATE_PLUGIN_LINK OFF)
set(SMTG_RUN_VST_VALIDATOR OFF)
set(SMTG_CREATE_MODULE_INFO OFF)

function (zrythm_setup_vst3sdk_targets)
  list(APPEND CMAKE_MODULE_PATH "${vst3sdk_SOURCE_DIR}/cmake/modules")
  include(SMTG_VST3_SDK)
  set(SDK_ROOT "${vst3sdk_SOURCE_DIR}")
  set(public_sdk_SOURCE_DIR "${SDK_ROOT}/public.sdk")
  set(pluginterfaces_SOURCE_DIR "${SDK_ROOT}/pluginterfaces")
  smtg_create_lib_base_target()
  smtg_create_pluginterfaces_target()
  smtg_create_public_sdk_common_target()
  smtg_create_public_sdk_target()
  smtg_create_public_sdk_hosting_target()
endfunction ()
zrythm_setup_vst3sdk_targets()

# Consumed by smtg_target_add_library_main() (via smtg_add_vst3plugin()) and
# by the validator's CMakeLists.txt - set at directory scope so subdirectories
# can see them
set(SDK_ROOT "${vst3sdk_SOURCE_DIR}")
set(public_sdk_SOURCE_DIR "${SDK_ROOT}/public.sdk")
set(pluginterfaces_SOURCE_DIR "${SDK_ROOT}/pluginterfaces")

foreach (vst3sdk_tgt base pluginterfaces sdk_common sdk sdk_hosting)
  zrythm_target_disable_warnings(${vst3sdk_tgt})
endforeach ()
target_link_libraries(base PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # threadchecker_linux.cpp uses std::terminate() without including <exception>
  # (fails with libc++; not fixed upstream as of v3.8.0_build_66)
  target_compile_options(sdk_common PRIVATE -include exception)
endif ()
