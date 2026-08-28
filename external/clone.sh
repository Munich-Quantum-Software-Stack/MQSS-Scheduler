#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Clone all scheduler dependencies into external/src/
# Usage: bash clone.sh [--update]
#   --update  Pull latest changes if repos already exist
# ------------------------------------------------------------------------------
set -euo pipefail

DEPS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

UPDATE=false
if [[ "${1:-}" == "--update" ]]; then
    UPDATE=true
fi

clone_or_update() {
    local name="$1"
    local url="$2"
    local ref="$3"       # branch or tag
    local ref_type="$4"  # "branch" or "tag"

    local dest="$name"

    if [ -d "$dest/.git" ]; then
        if $UPDATE; then
            echo "[update] $name"
            git -C "$dest" fetch --tags origin
            git -C "$dest" checkout "$ref"
            if [ "$ref_type" = "branch" ]; then
                git -C "$dest" pull origin "$ref"
            fi
        else
            echo "[skip]   $name  (already cloned, use --update to refresh)"
        fi
    else
        echo "[clone]  $name  ($url @ $ref)"
        if [ "$ref_type" = "tag" ]; then
            git -c advice.detachedHead=false clone --depth 1 --branch "$ref" "$url" "$dest"
        else
            git clone --branch "$ref" "$url" "$dest"
        fi
    fi
}

# ------------------------------------------------------------------------------
# Dependencies (order matters: QDMI first, then QInfo, submitter)
# Note: CxxQDMI is bundled locally as QDMI-Device-Example — no clone needed.
# ------------------------------------------------------------------------------

# Pinned to a release tag rather than develop, so this repo's QDMI version
# doesn't silently drift. QDMI-Device-Example (bundled below, no clone
# needed) has been verified/fixed to build against this specific tag — see
# its qdmi_site.cpp/qdmi_lib.hpp/qdmi.hpp/qdmi_job.cpp for the two API
# changes since v1.1.0 (QDMI_SITE_PROPERTY_ID renamed to
# QDMI_SITE_PROPERTY_INDEX; QDMI_job_wait gained a `size_t timeout`
# parameter). Bumping this tag needs re-checking both against the new
# version's include/qdmi/{client,constants,device}.h.
clone_or_update \
    "QDMI" \
    "https://github.com/Munich-Quantum-Software-Stack/QDMI.git" \
    "v1.3.2" \
    "tag"

clone_or_update \
    "QInfo" \
    "https://github.com/Munich-Quantum-Software-Stack/QInfo.git" \
    "develop" \
    "branch"

clone_or_update \
    "submitter" \
    "https://github.com/Munich-Quantum-Software-Stack/submitter.git" \
    "minh/ssintegrat" \
    "branch"

echo ""
echo "All sources are in: $DEPS_DIR"
echo "Run build.sh next to compile and install them."
