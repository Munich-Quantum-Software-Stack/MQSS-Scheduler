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

# Script to run Slurm SPANK plugin tests inside a pre-configured Docker container.
set -euo pipefail

# Helper to dump log files in case of failure
show_logs() {
  echo "=== Slurmctld Log ==="
  sudo cat /var/log/slurmctld.log || true
  echo "=== Slurmd Log ==="
  sudo cat /var/log/slurmd.log || true
}

# If any command fails, dump logs before exiting
trap show_logs ERR

echo "=== Verifying basic Slurm srun connectivity ==="
srun --partition=mqss --immediate=5 /bin/true

# noxfile.py lives directly at /workspace (WORKDIR), so nox auto-discovers it
# without needing to cd anywhere first.

# Running smoke tests
# No real MQSS service is available locally, so we validate env injection
# against a fake, non-resolving base URL instead of a real endpoint.
echo "=== Running SPANK smoke tests ==="
uvx nox -s smoke_tests -- \
  --partition mqss \
  --hook-mode full \
  --require-all-hooks \
  --test-base-url "http://mock-mqss.invalid"

# Disable the error trap before exiting cleanly
trap - ERR
echo "=== All SPANK tests completed successfully! ==="
