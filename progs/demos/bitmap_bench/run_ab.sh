#!/usr/bin/env bash
#
# Build bitmap_bench and run the pixel-store A/B.
#
# bitmap_bench itself measures both strategies (it re-runs itself once per
# strategy, because src/fg_font.c caches the choice per process), so this
# script only exists to get the build configuration right -- notably
# -DFREEGLUT_COCOA=ON on macOS, since an X11/XQuartz build would benchmark a
# different backend entirely.
#
# Usage:
#   progs/demos/bitmap_bench/run_ab.sh [-b BUILD_DIR] [bitmap_bench args...]
#
# Examples:
#   progs/demos/bitmap_bench/run_ab.sh
#   progs/demos/bitmap_bench/run_ab.sh --seconds 5 --font 9x15
#   progs/demos/bitmap_bench/run_ab.sh --strategy getset --hold   # eyeball it

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
BUILD="${ROOT}/build-bitmap-bench"

if [ "${1-}" = "-b" ]; then
    BUILD="$2"
    shift 2
fi

JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE=Release
    -DFREEGLUT_BUILD_DEMOS=ON
    -DFREEGLUT_BUILD_SHARED_LIBS=ON
    -DFREEGLUT_BUILD_STATIC_LIBS=OFF
)
if [ "$(uname -s)" = "Darwin" ]; then
    CMAKE_ARGS+=( -DFREEGLUT_COCOA=ON )
fi

echo "=== building in ${BUILD} ==="
cmake -S "$ROOT" -B "$BUILD" "${CMAKE_ARGS[@]}" >/dev/null
cmake --build "$BUILD" --target bitmap_bench pixel_store_check -j"$JOBS" >/dev/null

echo "=== pixel-store state is preserved by both paths ==="
for s in clientattrib getset; do
    printf '  %-13s ' "$s"
    if FREEGLUT_BITMAP_PIXEL_STORE="$s" "${BUILD}/bin/pixel_store_check" >/dev/null; then
        echo "restore OK"
    else
        echo "RESTORE FAILED"
        exit 1
    fi
done

echo "=== benchmark ==="
"${BUILD}/bin/bitmap_bench" "$@"
