#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Copyright 2024 - 2026 Munich Quantum Software Stack
# All rights reserved.
#
# Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://llvm.org/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

# Entrypoint script for starting Slurm SPANK test container services.
set -euo pipefail

# Detect bind-mounted workspace and rebuild/reinstall plugin if present
# When bind-mounted, the sentinel file `/workspace/.built-in-image` will be shadowed and missing.
if [[ ! -f /workspace/.built-in-image ]]; then
  echo "=== Bind-mounted workspace detected. Rebuilding/reinstalling SPANK plugin... ==="

  # Unlike the plugin .so (rebuilt below) and test/ (read live from the mount),
  # /etc/slurm/*.conf is only ever populated by the Dockerfile's COPY at image
  # build time — the bind mount never touches /etc/slurm/ directly. Refresh it
  # here so editing slurm-config/*.conf only needs a container restart, not a
  # full image rebuild, same as the plugin source.
  echo "=== Refreshing /etc/slurm/*.conf from bind-mounted slurm-config/ ==="
  sudo cp /workspace/slurm-config/slurm.conf /etc/slurm/slurm.conf
  sudo cp /workspace/slurm-config/cgroup.conf /etc/slurm/cgroup.conf
  sudo cp /workspace/slurm-config/gres.conf /etc/slurm/gres.conf

  SLURM_LIB_DIR=$(dpkg-architecture -qDEB_HOST_MULTIARCH | xargs -I{} echo /usr/lib/{}/slurm-wlm)
  cmake -S /workspace -B /workspace/build-spank-docker -DCMAKE_BUILD_TYPE=Release -DBUILD_MQSS_SPANK=ON \
    -DMQSS_SPANK_INSTALL_DIR="$SLURM_LIB_DIR" -DMQSS_SPANK_SLURM_CONF_DIR=/etc/slurm
  cmake --build /workspace/build-spank-docker --target mqss-spank-plugin
  sudo cmake --install /workspace/build-spank-docker --component mqss-spank-plugin
  rm -rf /workspace/build-spank-docker

  # The CMake installation writes a commented-out template of mqss-spank.conf.
  # We must activate it by writing the uncommented, active required line.
  # MQSS_BASE_URL only needs to be non-empty for local testing (no real MQSS
  # token available); the plugin never dials out to it itself, it only injects
  # the value into the job environment.
  echo "required $SLURM_LIB_DIR/mqss-spank-plugin.so mqss_base_url=http://mock-mqss.invalid mqss_license_prefix=mqss_" \
    | sudo tee /etc/slurm/plugstack.conf.d/mqss-spank.conf
fi

# Start Munge service
echo "=== Starting Munge service ==="
sudo service munge start

# Start Slurmctld
echo "=== Starting Slurmctld ==="
sudo /usr/sbin/slurmctld || { echo "ERROR: slurmctld failed to start"; exit 1; }

# Start Slurmd
echo "=== Starting Slurmd ==="
sudo /usr/sbin/slurmd -N localhost || { echo "ERROR: slurmd failed to start"; exit 1; }

# Wait for node to be ready
echo "=== Waiting for Slurm node to become ready ==="
for i in {1..30}; do
  if sinfo -h -n "localhost" -o "%t" | grep -qE "idle|alloc"; then
    break
  fi
  # Force resume if the node got marked DOWN/DRAINED
  sudo scontrol update nodename="localhost" state=resume 2>/dev/null || true
  sleep 1
done

if ! sinfo -h -n "localhost" -o "%t" | grep -qE "idle|alloc"; then
  echo "ERROR: Node did not become ready after 30 seconds"
  exit 1
fi

exec "$@"