# ------------------------------------------------------------------------------
# Source this file to set up the environment for running the scheduler examples.
# All MQSS packages (QDMI, QDMI-Device-Example, QInfo, submitter) are expected to be
# installed under scheduler/external/installed/.
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
DEPS_INSTALLED="$SCHEDULER_ROOT/external/installed"

# LLVM is built separately (top-level shared install)
LLVM_ROOT="${LLVM_ROOT:-/home/admin/shared/dependencies/installed}"

LLVM_INC="$LLVM_ROOT/include"
LLVM_LIB="$LLVM_ROOT/lib"

QDMI_EXAMPLE_DRIVER="$SCHEDULER_ROOT/external/QDMI/build/examples/driver"

# Generate qdmi.conf so the QDMI driver knows which device libraries to
# load — one line per QC alias (external/qdmi_device_aliases.txt),
# each an independently-built fake device installed alongside everything
# else in $DEPS_INSTALLED/lib (see external/build.sh and
# external/generate_qdmi_device_variants.sh) so different-alias jobs can
# execute concurrently instead of serializing through one device/Submitter.
_LIB_EXT=$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" )
_QDMI_CONF="$DEPS_INSTALLED/qdmi.conf"
: > "$_QDMI_CONF"
while IFS= read -r _alias; do
    [[ "$_alias" =~ ^[[:space:]]*#.*$ || -z "${_alias// /}" ]] && continue
    _prefix=$(echo "$_alias" | tr '[:lower:]' '[:upper:]')
    echo "$DEPS_INSTALLED/lib/lib${_alias}-qdmi-device.$_LIB_EXT $_prefix read_write" >> "$_QDMI_CONF"
done < "$SCHEDULER_ROOT/external/qdmi_device_aliases.txt"
export QDMI_CONF="$_QDMI_CONF"
unset _LIB_EXT _QDMI_CONF _alias _prefix

# Real (Aer) execution for the fake devices: <PREFIX>_QDMI_run_on_simulator
# (generated from CXX_QDMI_run_on_simulator in cxx_device.cpp) shells out to
# this PEP723 script via `uv run --script` when set - missing/failing is a
# graceful fallback to random results, not a hard error, so this is safe to
# always export. The same script is shared unmodified by every device
# variant (see generate_qdmi_device_variants.sh - no prefix-specific logic
# in it), and lives in the *source* tree, never copied anywhere by the
# CMake build.
export MQSS_SIM_SCRIPT="$SCHEDULER_ROOT/external/QDMI/examples/device/src/run_circuit.py"

# Export CMake-relevant roots
export LLVM_ROOT
export QDMI_ROOT="$DEPS_INSTALLED"
export QDMI_DEVICE_EXAMPLE_ROOT="$DEPS_INSTALLED"
export QINFO_ROOT="$DEPS_INSTALLED"
export SUBMITTER_ROOT="$DEPS_INSTALLED"

# Compiler and linker search paths
export CPATH="$DEPS_INSTALLED/include:$SCHEDULER_INSTALLED_DIR/include:$LLVM_INC${CPATH:+:$CPATH}"
export INCLUDE="$DEPS_INSTALLED/include:$SCHEDULER_INSTALLED_DIR/include:$LLVM_INC${INCLUDE:+:$INCLUDE}"
export LD_LIBRARY_PATH="$DEPS_INSTALLED/lib:$SCHEDULER_INSTALLED_DIR/lib:$SCHEDULER_INSTALLED_DIR/lib64:$LLVM_LIB:$QDMI_EXAMPLE_DRIVER:/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBRARY_PATH="$DEPS_INSTALLED/lib:$SCHEDULER_INSTALLED_DIR/lib:$SCHEDULER_INSTALLED_DIR/lib64:$LLVM_LIB:$QDMI_EXAMPLE_DRIVER:/usr/local/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"

# Make LLVM tools available on PATH. MPI itself needs no entry here on
# Ubuntu - its openmpi-bin package puts mpicxx/mpirun on PATH directly,
# unlike Rocky's /usr/lib64/openmpi/bin layout.
export PATH="$LLVM_ROOT/bin${PATH:+:$PATH}"

export MQSS_ENVS_LOADED=1
