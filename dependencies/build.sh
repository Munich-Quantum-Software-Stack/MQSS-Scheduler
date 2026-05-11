#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Build and install all scheduler dependencies into dependencies/installed/
# Prerequisite: run clone.sh first.
#
# Usage: bash build.sh [--jobs N] [--force]
#   --jobs N  Number of parallel make jobs (default: number of CPU cores)
#   --force   Rebuild all packages even if already installed
#
# Build order: QDMI → FakeQDMI-Device-Example → QInfo → submitter
# LLVM is expected to already be installed (see the top-level dependencies/
# scripts or provide LLVM_ROOT below).
# ------------------------------------------------------------------------------
set -euo pipefail

DEPS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$DEPS_DIR"
INSTALL_DIR="$DEPS_DIR/installed"
echo "Dependencies build script: DEPS_DIR=$DEPS_DIR"
echo "Source dependencies from: SRC_DIR=$SRC_DIR"
echo "Installing dependencies to: INSTALL_DIR=$INSTALL_DIR"
echo "Note: LLVM is expected to already be installed.
    If not, build it first or set LLVM_ROOT to point to your LLVM installation."

# LLVM: point to the pre-built LLVM installation (built by top-level
# dependencies/scripts/). Override with LLVM_ROOT env var if needed.
LLVM_ROOT="${LLVM_ROOT:-/home/admin/shared/dependencies/installed}"
echo "Using LLVM_ROOT=$LLVM_ROOT"

JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
FORCE=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        --jobs)  JOBS="$2"; shift 2 ;;
        --force) FORCE=true; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

mkdir -p "$INSTALL_DIR"

# Check whether a sentinel file already exists in installed/ to skip a rebuild.
# Usage: is_installed <relative-path-under-installed>
is_installed() {
    [[ "$FORCE" == false ]] && [[ -e "$INSTALL_DIR/$1" ]]
}

build_cmake() {
    local name="$1"
    local src="$2"
    local sentinel="$3"   # file to check in installed/ (e.g. include/qdmi/client.h)
    shift 3

    if is_installed "$sentinel"; then
        echo ""
        echo "[skip]   $name already installed ($INSTALL_DIR/$sentinel exists)"
        echo "         Run with --force to rebuild."
        return 0
    fi

    echo ""
    echo "========================================"
    echo " Building: $name"
    echo "========================================"

    local build_dir="$src/build"
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
# 1. QDMI  (no CMAKE_PREFIX_PATH — prevents QDMI from finding itself)
# ------------------------------------------------------------------------------
build_cmake "QDMI" "$SRC_DIR/QDMI" "include/qdmi/client.h" \
    -DBUILD_QDMI_TESTS=OFF \
    -DBUILD_QDMI_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_QDMI_TEMPLATES=OFF \
    -DBUILD_QDMI_DOCS=OFF

# ------------------------------------------------------------------------------
# 2. FakeQDMI-Device-Example  (bundled locally, depends on QDMI)
# ------------------------------------------------------------------------------
build_cmake "FakeQDMI-Device-Example" "$DEPS_DIR/FakeQDMI-Device-Example" "include/qdmi.hpp" \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
    -DQDMI_ROOT="$INSTALL_DIR"

# ------------------------------------------------------------------------------
# 3. QInfo
# ------------------------------------------------------------------------------
build_cmake "QInfo" "$SRC_DIR/QInfo" "include/qinfo/qinfo.h" \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
    -DBUILD_QINFO_TESTS=OFF \
    -DBUILD_SHARED_LIBS=ON \
    -DQINFO_INSTALL=ON \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON

# ------------------------------------------------------------------------------
# 4. submitter  (depends on QDMI, FakeQDMI-Device-Example, QInfo, LLVM)
# ------------------------------------------------------------------------------
build_cmake "submitter" "$SRC_DIR/submitter" "include/submitter.hpp" \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
    -DQDMI_ROOT="$INSTALL_DIR" \
    -DFAKEQDMI_DEV_ROOT="$INSTALL_DIR" \
    -DQINFO_ROOT="$INSTALL_DIR" \
    -DLLVM_ROOT="$LLVM_ROOT" \
    -DLLVM_DIR="$LLVM_ROOT/lib/cmake/llvm" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON

echo ""
echo "========================================"
echo " All dependencies installed to:"
echo "   $INSTALL_DIR"
echo "========================================"
