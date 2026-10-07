#!/bin/bash

# Builds FlashFloppy on Linux from the source tree that holds this script, as an out-of-tree
# build: everything the build writes goes into a build directory of its own. The tools come
# from the directory filled by private/set_up_tools.sh.
#
# Usage: build.sh [make-argument...]
#
# With no arguments builds the "dist" goal: the complete release tree with the .upd files, in
# out/flashfloppy-<version>/ of the build directory. The version is the short hash of the
# checked-out commit unless VER=... is among the arguments.

# Written with the help of Claude Fable 5.1.

set -o pipefail

: "${BUILD_DIR="${HOME}/retro/flashfloppy-build"}"  # May be deleted for a clean rebuild.
: "${TOOLS_DIR="${HOME}/retro/flashfloppy-tools"}"

fail() {
  echo "ERROR: $*" >&2
  exit 1
}

main() {
  local src_dir
  src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)" \
      || fail "Cannot locate the source tree."
  local -a make_args=("$@")

  source "${TOOLS_DIR}/env.sh" \
      || fail "No tools in ${TOOLS_DIR}/ - run private/set_up_tools.sh first."

  if (( ${#make_args[@]} == 0 )); then
    make_args=(dist)
  fi

  echo "Building ${src_dir}/ in ${BUILD_DIR}/"
  make -C "${src_dir}" O="${BUILD_DIR}" -j "$(nproc)" "${make_args[@]}" \
      || fail "The build failed."
}

main "$@"
