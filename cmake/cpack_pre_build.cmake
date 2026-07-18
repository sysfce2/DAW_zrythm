# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: CC0-1.0

# Remove shell completions from AppImage — they conflict with AppRun
# detection (the bash completion file is named "zrythm", same as the
# binary) and are useless inside a portable image anyway.
# Also fix AppRun — the generator picks the wrong file named "zrythm"
# in the install tree (e.g., share/zrythm directory) instead of bin/zrythm.
if(CPACK_GENERATOR STREQUAL "AppImage")
  file(REMOVE_RECURSE "${CPACK_TEMPORARY_DIRECTORY}/share/bash-completion")
  file(REMOVE_RECURSE "${CPACK_TEMPORARY_DIRECTORY}/share/fish")
  file(WRITE "${CPACK_TEMPORARY_DIRECTORY}/AppRun"
    "#!/usr/bin/env bash\nset -e\nthis_dir=\"$(readlink -f \"$(dirname \"$0\")\")\"\nexec \"$this_dir/bin/zrythm\" \"$@\"\n")
endif()
