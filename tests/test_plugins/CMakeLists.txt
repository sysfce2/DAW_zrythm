# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Minimal VST3/CLAP test fixtures written directly against the official SDKs.
# These replace the old faust2juce-generated bundled plugins in tests.

# MSVC defaults __cplusplus to 201703L; the VST3 SDK's SMTG_CPP20 guard needs
# the real value, else module_win32.cpp passes std::u8string where std::string
# is expected.
if(MSVC)
  add_compile_options(/Zc:__cplusplus)
endif()

# VST3 validator from the SDK (provides the `validator` target)
add_subdirectory(
  ${vst3sdk_SOURCE_DIR}/public.sdk/samples/vst-hosting/validator
  ${CMAKE_CURRENT_BINARY_DIR}/vst3-validator
  EXCLUDE_FROM_ALL
)
zrythm_target_disable_warnings(validator)

# moduleinfotool from the SDK (generates moduleinfo.json inside the bundles
# below, so hosts can scan them without instantiating them)
add_subdirectory(
  ${vst3sdk_SOURCE_DIR}/public.sdk/samples/vst-utilities/moduleinfotool
  ${CMAKE_CURRENT_BINARY_DIR}/vst3-moduleinfotool
  EXCLUDE_FROM_ALL
)
zrythm_target_disable_warnings(moduleinfotool)

# Place the plugin bundles under this directory
# (<binary_dir>/VST3/<config>/<name>.vst3 and <binary_dir>/CLAP/<name>.clap)
set(SMTG_CUSTOM_BINARY_LOCATION ${CMAKE_CURRENT_BINARY_DIR})

set(vst3_fixture_extra_sources
  ${SDK_ROOT}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
)

# Generate moduleinfo.json only for Test Gain so that host scanning covers
# both code paths: the fast path (descriptions from moduleinfo.json) and the
# slow path (instantiating the plugin during scanning)
set(SMTG_CREATE_MODULE_INFO ON)
smtg_add_vst3plugin(test_gain_vst3
  PACKAGE_NAME "Test Gain"
  SOURCES_LIST test_gain_vst3.cpp ${vst3_fixture_extra_sources}
)
set(SMTG_CREATE_MODULE_INFO OFF)
smtg_add_vst3plugin(test_synth_vst3
  PACKAGE_NAME "Test Synth"
  SOURCES_LIST test_synth_vst3.cpp ${vst3_fixture_extra_sources}
)

foreach(vst3_fixture test_gain_vst3 test_synth_vst3)
  target_link_libraries(${vst3_fixture} PRIVATE sdk)
  target_include_directories(${vst3_fixture} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  zrythm_target_disable_warnings(${vst3_fixture})
  set_target_properties(${vst3_fixture} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
  )
  zrythm_fix_vst3_ninja_bundle_output_dir(${vst3_fixture})
endforeach()

add_library(test_gain_clap MODULE test_gain_clap.cpp)
add_library(test_synth_clap MODULE test_synth_clap.cpp)

foreach(clap_fixture test_gain_clap test_synth_clap)
  target_link_libraries(${clap_fixture} PRIVATE clap-helpers)
  target_include_directories(${clap_fixture} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  zrythm_target_disable_warnings(${clap_fixture})
  set_target_properties(${clap_fixture} PROPERTIES
    PREFIX ""
    SUFFIX ".clap"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/CLAP/$<CONFIG>"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
  )
endforeach()

add_custom_target(zrythm_test_plugins
  DEPENDS
    test_gain_vst3
    test_synth_vst3
    test_gain_clap
    test_synth_clap
    validator
)

# Run the VST3 SDK validator on each test bundle (the validator requires the
# bundle directory, not the module inside it)
add_test(
  NAME vst3_validator_test_gain
  COMMAND $<TARGET_FILE:validator>
          "$<TARGET_PROPERTY:test_gain_vst3,SMTG_PLUGIN_PACKAGE_PATH>"
)
add_test(
  NAME vst3_validator_test_synth
  COMMAND $<TARGET_FILE:validator>
          "$<TARGET_PROPERTY:test_synth_vst3,SMTG_PLUGIN_PACKAGE_PATH>"
)
