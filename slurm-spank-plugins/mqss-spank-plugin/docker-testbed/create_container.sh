#!/usr/bin/env bash
set -euo pipefail

# Resolve paths from this script's own location, not the caller's $PWD, so it
# works whether you run it from docker-testbed/ (where it lives) or elsewhere.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

IMAGE_TAG="mqss-slurm-testbed-img"
CONTAINER_NAME="mqss-slurm-testbed"
NETWORK_NAME="mqss-net"

docker build -f "$SCRIPT_DIR/Dockerfile" -t "$IMAGE_TAG" "$PLUGIN_DIR"

# Shared user-defined network so jobs submitted here can resolve the
# grpc-server-example container by name (see
# ../../../examples/grpc-server-example/docker-testbed/create_container.sh) — needed for
# test/submit_mqss_job_offload.sbatch. User-defined networks (unlike the
# default bridge) support container-name-based DNS resolution.
docker network create "$NETWORK_NAME" >/dev/null 2>&1 || true

# Remove a leftover container from a previous run, if any, so --name doesn't
# collide with a stopped-but-not-yet-removed container of the same name.
docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true

# Run detached with the image's real ENTRYPOINT intact (don't override it with
# --entrypoint /bin/bash — that skips entrypoint.sh entirely, so munge/
# slurmctld/slurmd never start and sinfo fails with "connect failure"
# inside the shell). Overriding just the CMD to `sleep infinity` still lets
# entrypoint.sh start the services and wait for the node to be ready first.
docker run -d --rm --name "$CONTAINER_NAME" --network "$NETWORK_NAME" \
  -v "$PLUGIN_DIR":/workspace "$IMAGE_TAG" sleep infinity

echo "Waiting for Slurm services to start..."
until docker exec "$CONTAINER_NAME" sinfo -h -n localhost -o "%t" 2>/dev/null | grep -qE "idle|alloc"; do
  sleep 1
done

# Attach an interactive shell to the now-running container.
docker exec -it "$CONTAINER_NAME" bash
