#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Clone all scheduler dependencies into dependencies/src/
# Usage: bash clone.sh [--update]
#   --update  Pull latest changes if repos already exist
# ------------------------------------------------------------------------------
set -euo pipefail

DEPS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$DEPS_DIR/src"
mkdir -p "$SRC_DIR"

UPDATE=false
if [[ "${1:-}" == "--update" ]]; then
    UPDATE=true
fi

clone_or_update() {
    local name="$1"
    local url="$2"
    local ref="$3"       # branch or tag
    local ref_type="$4"  # "branch" or "tag"

    local dest="$SRC_DIR/$name"

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
            git clone --depth 1 --branch "$ref" "$url" "$dest"
        else
            git clone --branch "$ref" "$url" "$dest"
        fi
    fi
}

# ------------------------------------------------------------------------------
# Dependencies (order matters: QDMI first, then QInfo, submitter)
# Note: CxxQDMI is bundled locally as FakeQDMI-Device-Example — no clone needed.
# ------------------------------------------------------------------------------

clone_or_update \
    "QDMI" \
    "https://github.com/Munich-Quantum-Software-Stack/QDMI.git" \
    "v1.1.0" \
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
echo "All sources are in: $SRC_DIR"
echo "Run build.sh next to compile and install them."
