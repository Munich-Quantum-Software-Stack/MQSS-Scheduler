#!/usr/bin/env bash
set -euo pipefail

# Resolve paths from this script's own location, not the caller's $PWD, so it
# works whether you run it from docker-testbed/ (where it lives) or elsewhere.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

IMAGE_TAG="mqss-slurm-testbed-img"
CONTAINER_NAME="mqss-slurm-testbed"

# Remove a leftover container from a previous run, if any, so --name doesn't
# collide with a stopped-but-not-yet-removed container of the same name.
docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true
docker rmi -f "$IMAGE_TAG" >/dev/null 2>&1 || true
