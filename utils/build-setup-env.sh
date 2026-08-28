# shellcheck shell=bash
# ------------------------------------------------------------------------------
# Source this file to set up the environment for building and running the
# scheduler. All MQSS packages (QDMI, QDMI-Device-Example, QInfo, submitter) are
# expected to be installed under scheduler/external/installed/.
#
# Usage:
#   source utils/build-setup-env.sh
#
# LLVM is pre-built externally; override LLVM_ROOT if needed:
#   LLVM_ROOT=/path/to/llvm source utils/build-setup-env.sh
# ------------------------------------------------------------------------------

# Avoid re-exporting if already loaded
[ -n "$MQSS_ENVS_LOADED" ] && return 0

# Directory of this script → scheduler root is one level up
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCHEDULER_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SCHEDULER_INSTALLED_DIR="$SCHEDULER_ROOT/install"

# All MQSS dependencies live here
DEPS_INSTALLED="$SCHEDULER_ROOT/external/installed"

# LLVM is built separately (top-level shared install)
LLVM_ROOT="${LLVM_ROOT:-/home/admin/shared/dependencies/installed}"

# Individual include / lib paths
QDMI_INC="$DEPS_INSTALLED/include"
QDMI_LIB="$DEPS_INSTALLED/lib"

QDMI_DEVICE_EXAMPLE_INC="$DEPS_INSTALLED/include"
QDMI_DEVICE_EXAMPLE_LIB="$DEPS_INSTALLED/lib"

QINFO_INC="$DEPS_INSTALLED/include"
QINFO_LIB="$DEPS_INSTALLED/lib"

SUBMITTER_INC="$DEPS_INSTALLED/include"
SUBMITTER_LIB="$DEPS_INSTALLED/lib"

LLVM_INC="$LLVM_ROOT/include"
LLVM_LIB="$LLVM_ROOT/lib"

# Export CMake-relevant roots
export LLVM_ROOT
export QDMI_ROOT="$DEPS_INSTALLED"
export QDMI_DEVICE_EXAMPLE_ROOT="$DEPS_INSTALLED"
export QINFO_ROOT="$DEPS_INSTALLED"
export SUBMITTER_ROOT="$DEPS_INSTALLED"

# Compiler and linker search paths
# All MQSS packages share the same prefix, so one entry covers QDMI/QDMI-Device-Example/QInfo/submitter
export CPATH="$DEPS_INSTALLED/include:$SCHEDULER_INSTALLED_DIR/include:$LLVM_INC${CPATH:+:$CPATH}"
export INCLUDE="$DEPS_INSTALLED/include:$SCHEDULER_INSTALLED_DIR/include:$LLVM_INC${INCLUDE:+:$INCLUDE}"
QDMI_EXAMPLE_DRIVER="$SCHEDULER_ROOT/external/QDMI/build/examples/driver"
QDMI_EXAMPLE_DEVICE="$SCHEDULER_ROOT/external/QDMI/build/examples/device/cxx"

# Real-execution backend for the cxx example device's job_submit: a small Aer
# subprocess (source tree, not the build dir — it's a script, nothing to
# build). See external/QDMI/examples/device/cxx/run_circuit.py and
# device.cpp's CXX_QDMI_run_on_simulator. Unset/unreachable => job_submit
# silently falls back to its original random-result behavior.
export MQSS_SIM_SCRIPT="$SCHEDULER_ROOT/external/QDMI/examples/device/cxx/run_circuit.py"

# Generate qdmi.conf so the QDMI driver knows which device library to load
_LIB_EXT=$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" )
_QDMI_CONF="$QDMI_EXAMPLE_DEVICE/qdmi.conf"
echo "$QDMI_EXAMPLE_DEVICE/libcxx_device.$_LIB_EXT CXX read_write" > "$_QDMI_CONF"
export QDMI_CONF="$_QDMI_CONF"
unset _LIB_EXT _QDMI_CONF

export LD_LIBRARY_PATH="$DEPS_INSTALLED/lib:$SCHEDULER_INSTALLED_DIR/lib:$SCHEDULER_INSTALLED_DIR/lib64:$LLVM_LIB:$QDMI_EXAMPLE_DRIVER:$QDMI_EXAMPLE_DEVICE:/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBRARY_PATH="$DEPS_INSTALLED/lib:$SCHEDULER_INSTALLED_DIR/lib:$SCHEDULER_INSTALLED_DIR/lib64:$LLVM_LIB:$QDMI_EXAMPLE_DRIVER:$QDMI_EXAMPLE_DEVICE:/usr/local/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"

# Make LLVM tools (clang, opt, llc, …) available on PATH
export PATH="$LLVM_ROOT/bin${PATH:+:$PATH}"

export MQSS_ENVS_LOADED=1
