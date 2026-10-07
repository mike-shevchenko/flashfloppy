#!/bin/bash

# Builds ffemu from the source tree that holds this script, as an out-of-tree build. In MSYS2
# the build directory is flashfloppy-build/ next to the source tree, on Linux the one of the
# firmware's build.sh; the program lands in out/ffemu/ of it.
#
# Usage: ffemu/build.sh [make-argument...]

set -o pipefail

fail() {
  echo "ERROR: $*" >&2
  exit 1
}

main() {
  local src_dir
  src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)" \
      || fail "Cannot locate the source tree."

  # The Windows and the Linux objects have the same names, so each needs a directory of its own.
  local build_dir="${BUILD_DIR:-}"
  if [[ -z "${build_dir}" ]]; then
    case "$(uname -o)" in
      Msys|Cygwin) build_dir="${src_dir}-build" ;;
      *) build_dir="${HOME}/retro/flashfloppy-build" ;;
    esac
  fi

  echo "Building ${src_dir}/ffemu/ in ${build_dir}/"
  make -C "${src_dir}/ffemu" O="${build_dir}" -j "$(nproc)" "$@" || fail "The build failed."
}

main "$@"
