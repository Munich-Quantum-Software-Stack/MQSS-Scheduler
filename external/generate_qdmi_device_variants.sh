#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Generate one independent, buildable copy of external/QDMI/examples/device/
# per QC alias listed in qdmi_device_aliases.txt, each with its own symbol
# prefix, CMake target name, and QDMI_DEVICE_PROPERTY_NAME so they can be
# built into distinct .so files and loaded together (see qdmi.conf).
#
# generate_prefixed_qdmi_headers() only rewrites the shared QDMI *headers* —
# cxx_device.cpp itself hardcodes "CXX_" as literal C++ identifier text (no
# macro/token-pasting indirection), so getting independent devices out of one
# source file requires actually text-substituting the .cpp, not just calling
# that CMake function again with a different prefix.
#
# Output lives under qdmi-device-variants/ — NOT inside the external/QDMI
# clone (which clone.sh --update can refresh/clobber at any time) and NOT
# committed to this repo's own git history either (regenerated fresh on every
# build, see external/build.sh) — deliberately, since these are generated
# copies of cxx_device.cpp, which itself carries a local, uncommitted Tier-A
# patch (real Aer execution) on top of upstream QDMI; regenerating fresh each
# build means there's never a stale, out-of-sync committed copy to reconcile.
#
# Usage: bash generate_qdmi_device_variants.sh
# Run automatically by build.sh, after QDMI's own build and before the
# per-variant device build loop — must run AFTER any local patches to
# cxx_device.cpp are in place, since it copies whatever is currently on disk.
# ------------------------------------------------------------------------------
set -euo pipefail

DEPS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DEVICE_DIR="$DEPS_DIR/QDMI/examples/device"
ALIASES_FILE="$DEPS_DIR/qdmi_device_aliases.txt"
OUT_ROOT="$DEPS_DIR/qdmi-device-variants"

if [ ! -d "$SRC_DEVICE_DIR" ]; then
    echo "error: $SRC_DEVICE_DIR not found - run clone.sh first" >&2
    exit 1
fi

mapfile -t ALIASES < <(grep -vE '^\s*#|^\s*$' "$ALIASES_FILE")

if [ "${#ALIASES[@]}" -eq 0 ]; then
    echo "error: no aliases found in $ALIASES_FILE" >&2
    exit 1
fi

mkdir -p "$OUT_ROOT"

for alias in "${ALIASES[@]}"; do
    prefix=$(echo "$alias" | tr '[:lower:]' '[:upper:]')
    prefix_lower=$(echo "$alias" | tr '[:upper:]' '[:lower:]')
    target_name="${alias}-qdmi-device"
    variant_dir="$OUT_ROOT/$alias"

    echo "[generate] $alias -> $variant_dir (prefix=$prefix, target=$target_name)"
    rm -rf "$variant_dir"
    mkdir -p "$variant_dir/src" "$variant_dir/cmake"

    cp "$SRC_DEVICE_DIR/CMakeLists.txt" "$variant_dir/CMakeLists.txt"
    cp "$SRC_DEVICE_DIR/src/CMakeLists.txt" "$variant_dir/src/CMakeLists.txt"
    cp "$SRC_DEVICE_DIR/src/cxx_device.cpp" "$variant_dir/src/cxx_device.cpp"
    cp "$SRC_DEVICE_DIR/src/run_circuit.py" "$variant_dir/src/run_circuit.py"
    cp "$SRC_DEVICE_DIR/cmake/ExternalDependencies.cmake" \
        "$variant_dir/cmake/ExternalDependencies.cmake"
    # Renamed to match ${QDMI_TARGET_NAME}-config.cmake.in, which
    # src/CMakeLists.txt's configure_package_config_file() call expects.
    cp "$SRC_DEVICE_DIR/cmake/cxx-qdmi-device-config.cmake.in" \
        "$variant_dir/cmake/${target_name}-config.cmake.in"

    # Top-level CMakeLists.txt: project/target name and QDMI_PREFIX.
    sed -i \
        -e "s/cxx-qdmi-device/${target_name}/g" \
        -e "s/QDMI_PREFIX \"CXX\"/QDMI_PREFIX \"${prefix}\"/" \
        "$variant_dir/CMakeLists.txt"

    # config.cmake.in hardcodes "cxx-qdmi-device" as a target/variable name
    # (TARGET check, include filename, _FIND_QUIETLY/_VERSION var names).
    sed -i "s/cxx-qdmi-device/${target_name}/g" \
        "$variant_dir/cmake/${target_name}-config.cmake.in"

    # cxx_device.cpp: the actual symbol/include/name substitution.
    #   - CXX_QDMI_* -> <PREFIX>_QDMI_* (~105 dlsym'd function/type names -
    #     must match the prefixed header generate_prefixed_qdmi_headers()
    #     produces, or symbol resolution and/or linkage silently breaks)
    #   - CXX_DEVICE_* -> <PREFIX>_DEVICE_* (file-scope const/constexpr,
    #     internal linkage by default in C++ so not strictly required for
    #     correctness, but keeps the generated file visually self-consistent)
    #   - include path -> the prefix-specific generated header directory
    #   - QDMI_DEVICE_PROPERTY_NAME literal -> the raw alias string, so
    #     Submitter ranks can select a device by qdmi::Device::get_name()
    #     doing a plain string-equality match against the alias (device
    #     ordering from get_devices() is not deterministic - see qrm-example)
    sed -i \
        -e "s/CXX_QDMI_/${prefix}_QDMI_/g" \
        -e "s/CXX_DEVICE_/${prefix}_DEVICE_/g" \
        -e "s#cxx_qdmi/device.h#${prefix_lower}_qdmi/device.h#" \
        -e "s/C++ Device with 5 qubits/${alias}/" \
        "$variant_dir/src/cxx_device.cpp"
done

echo "[generate] done: ${#ALIASES[@]} device variant(s) in $OUT_ROOT"
