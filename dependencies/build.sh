#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Build and install all scheduler dependencies into dependencies/installed/
# Prerequisite: run clone.sh first.
#
# Usage: bash build.sh [--jobs N]
#   --jobs N  Number of parallel make jobs (default: number of CPU cores)
#
# Build order: QDMI → FakeQDMI-Device-Example → QInfo → submitter
# LLVM is expected to already be installed (see the top-level dependencies/
# scripts or provide LLVM_ROOT below).
# ------------------------------------------------------------------------------
set -euo pipefail

DEPS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$DEPS_DIR/src"
INSTALL_DIR="$DEPS_DIR/installed"

# LLVM: point to the pre-built LLVM installation (built by top-level
# dependencies/scripts/). Override with LLVM_ROOT env var if needed.
LLVM_ROOT="${LLVM_ROOT:-/home/admin/shared/dependencies/installed}"

JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
while [[ $# -gt 0 ]]; do
    case "$1" in
        --jobs) JOBS="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

mkdir -p "$INSTALL_DIR"

build_cmake() {
    local name="$1"
    local src="$2"
    shift 2
    local build_dir="$src/build"

    echo ""
    echo "========================================"
    echo " Building: $name"
    echo "========================================"

    mkdir -p "$build_dir"
    cmake -S "$src" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DCMAKE_CXX_STANDARD=17 \
        "$@"
    cmake --build "$build_dir" --parallel "$JOBS"
    cmake --install "$build_dir"
    echo "[done]   $name installed to $INSTALL_DIR"
}

# ------------------------------------------------------------------------------
# 1. QDMI
# ------------------------------------------------------------------------------
build_cmake "QDMI" "$SRC_DIR/QDMI" \
    -DQDMI_BUILD_TESTS=OFF \
    -DQDMI_BUILD_EXAMPLES=OFF

# ------------------------------------------------------------------------------
# 2. FakeQDMI-Device-Example  (bundled locally, depends on QDMI)
# ------------------------------------------------------------------------------
build_cmake "FakeQDMI-Device-Example" "$DEPS_DIR/FakeQDMI-Device-Example" \
    -DQDMI_ROOT="$INSTALL_DIR"

# ------------------------------------------------------------------------------
# 3. QInfo
# ------------------------------------------------------------------------------
build_cmake "QInfo" "$SRC_DIR/QInfo" \
    -DBUILD_QINFO_TESTS=OFF

# ------------------------------------------------------------------------------
# 4. submitter  (depends on QDMI, FakeQDMI-Device-Example, QInfo, LLVM)
# ------------------------------------------------------------------------------
build_cmake "submitter" "$SRC_DIR/submitter" \
    -DQDMI_ROOT="$INSTALL_DIR" \
    -DCXXQDMI_ROOT="$INSTALL_DIR" \
    -DQINFO_ROOT="$INSTALL_DIR" \
    -DLLVM_ROOT="$LLVM_ROOT" \
    -DLLVM_DIR="$LLVM_ROOT/lib/cmake/llvm"

echo ""
echo "========================================"
echo " All dependencies installed to:"
echo "   $INSTALL_DIR"
echo "========================================"
