# ------------------------------------------------------------------------------
# Source this file to set up the environment for running the scheduler examples.
# All MQSS packages (QDMI, FakeQDMI-Dev, QInfo, submitter) are expected to be
# installed under scheduler/dependencies/installed/.
#
# Usage:
#   source examples/export_env_vars.sh
#
# LLVM is pre-built externally; override LLVM_ROOT if needed:
#   LLVM_ROOT=/path/to/llvm source examples/export_env_vars.sh
# ------------------------------------------------------------------------------

# Avoid re-exporting if already loaded
[ -n "$MQSS_ENVS_LOADED" ] && return 0

# Directory of this script → scheduler root is one level up
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCHEDULER_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SCHEDULER_INSTALLED_DIR="$SCHEDULER_ROOT/install"

# All MQSS dependencies live here
DEPS_INSTALLED="$SCHEDULER_ROOT/dependencies/installed"

# LLVM is built separately (top-level shared install)
LLVM_ROOT="${LLVM_ROOT:-/home/admin/shared/dependencies/installed}"

LLVM_INC="$LLVM_ROOT/include"
LLVM_LIB="$LLVM_ROOT/lib"

QDMI_EXAMPLE_DRIVER="$SCHEDULER_ROOT/dependencies/QDMI/build/examples/driver"
QDMI_EXAMPLE_DEVICE="$SCHEDULER_ROOT/dependencies/QDMI/build/examples/device/cxx"

# Generate qdmi.conf so the QDMI driver knows which device library to load
_LIB_EXT=$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" )
_QDMI_CONF="$QDMI_EXAMPLE_DEVICE/qdmi.conf"
echo "$QDMI_EXAMPLE_DEVICE/libcxx_device.$_LIB_EXT CXX read_write" > "$_QDMI_CONF"
export QDMI_CONF="$_QDMI_CONF"
unset _LIB_EXT _QDMI_CONF

# Export CMake-relevant roots
export LLVM_ROOT
export QDMI_ROOT="$DEPS_INSTALLED"
export FAKEQDMI_DEV_ROOT="$DEPS_INSTALLED"
export QINFO_ROOT="$DEPS_INSTALLED"
export SUBMITTER_ROOT="$DEPS_INSTALLED"

# Compiler and linker search paths
export CPATH="$DEPS_INSTALLED/include:$SCHEDULER_INSTALLED_DIR/include:$LLVM_INC${CPATH:+:$CPATH}"
export INCLUDE="$DEPS_INSTALLED/include:$SCHEDULER_INSTALLED_DIR/include:$LLVM_INC${INCLUDE:+:$INCLUDE}"
export LD_LIBRARY_PATH="$DEPS_INSTALLED/lib:$SCHEDULER_INSTALLED_DIR/lib:$SCHEDULER_INSTALLED_DIR/lib64:$LLVM_LIB:$QDMI_EXAMPLE_DRIVER:$QDMI_EXAMPLE_DEVICE:/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBRARY_PATH="$DEPS_INSTALLED/lib:$SCHEDULER_INSTALLED_DIR/lib:$SCHEDULER_INSTALLED_DIR/lib64:$LLVM_LIB:$QDMI_EXAMPLE_DRIVER:$QDMI_EXAMPLE_DEVICE:/usr/local/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"

# Make LLVM tools and MPI available on PATH
export PATH="$LLVM_ROOT/bin:/usr/lib64/openmpi/bin${PATH:+:$PATH}"

export MQSS_ENVS_LOADED=1
