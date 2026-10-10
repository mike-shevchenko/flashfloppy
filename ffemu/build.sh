#!/bin/bash

# Builds ffemu from the source tree that holds this script, as an out-of-tree build: the
# Shugart and the Apple II programs in parallel, each line of output prefixed by its target. In
# MSYS2 the build directory is flashfloppy-build/ next to the source tree, on Linux the one of
# the firmware's build.sh; the programs land in out/ffemu/ffemu and
# out/ffemu-apple2/ffemu-apple2 of it.
#
# Usage: ffemu/build.sh [make-argument...]

set -o pipefail

readonly TARGETS=(shugart apple2)

fail() {
  echo "ERROR: $*" >&2
  exit 1
}

# Usage: build_target <src_dir> <build_dir> <target> [make-argument...]
build_target() {
  local src_dir="$1"
  local build_dir="$2"
  local target="$3"
  shift 3
  make -C "${src_dir}/ffemu" O="${build_dir}" TARGET="${target}" -j "$(nproc)" "$@" 2>&1 \
      | sed -u "s/^/[${target}] /"
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
  local -A pids
  local target
  for target in "${TARGETS[@]}"; do
    build_target "${src_dir}" "${build_dir}" "${target}" "$@" &
    pids["${target}"]=$!
  done

  local failed=()
  for target in "${TARGETS[@]}"; do
    wait "${pids["${target}"]}" || failed+=("${target}")
  done
  (( ${#failed[@]} == 0 )) || fail "The build failed: ${failed[*]}."
}

main "$@"
