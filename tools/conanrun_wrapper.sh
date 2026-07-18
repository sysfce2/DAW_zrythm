#!/bin/bash
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# gdb exec-wrapper that sources Conan's runtime environment and
# re-applies sanitizer options that conanrun.sh overwrites.
#
# Usage as gdb exec-wrapper:
#   set exec-wrapper /path/to/conanrun_wrapper.sh <source_dir> <build_dir>
# gdb invokes: conanrun_wrapper.sh <source_dir> <build_dir> <program> <args...>

SOURCE_DIR="$1"
BUILD_DIR="$2"

if [[ ! -f "$BUILD_DIR/generators/conanrun.sh" ]]; then
  echo "Error: $BUILD_DIR/generators/conanrun.sh not found. Run 'conan install' first." >&2
  exit 1
fi
source "$BUILD_DIR/generators/conanrun.sh"

export ASAN_OPTIONS="strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:halt_on_error=1:abort_on_error=1:detect_leaks=0:suppressions=${SOURCE_DIR}/tools/asan_suppressions.supp"
export TSAN_OPTIONS="suppressions=${SOURCE_DIR}/tools/tsan_suppressions.supp:ignore_noninstrumented_modules=1"
export RTSAN_OPTIONS="suppressions=${SOURCE_DIR}/tools/rtsan_suppressions.supp"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:abort_on_error=1:suppressions=${SOURCE_DIR}/tools/ubsan_suppressions.supp"

# QML JIT is incompatible with ASan (shadow memory vs JIT-allocated code)
export QV4_FORCE_INTERPRETER=1

exec "${@:3}"
