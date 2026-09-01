#!/bin/bash
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

# Batch job: submit with
#   sbatch test/submit_mqss_job_offload.sbatch
# from /workspace inside the Slurm container. Unlike submit_mqss_job.sbatch
# (a stub that never leaves the container), this one actually offloads the
# circuit over gRPC to a separate container running examples/grpc-server-example/,
# which executes it on Aer and returns real measurement counts.
#
# Requires:
#   - The grpc-server-example container running and reachable by the hostname
#     below (both containers on the same Docker network — see
#     examples/grpc-server-example/Readme.md for how to wire this up).
#   - --mqss-base-url overrides the plugstack default (http://mock-mqss.invalid)
#     with the scheduler container's address; bell_state_offload.py derives
#     the gRPC host:port target directly from MQSS_BASE_URL.

#SBATCH --job-name=mqss-offload-job
#SBATCH --partition=mqss
#SBATCH --mqss-base-url=http://qc-container:50051
#SBATCH --mqss-qc-alias=aqt
#SBATCH --licenses=mqss_aqt:1
#SBATCH --gres=qpu:aqt:1
#SBATCH --output=mqss-offload-aqt-%j.out
#SBATCH --error=mqss-offload-aqt-%j.err

set -euo pipefail

# sbatch copies this script into a spool directory and runs it from there, so
# ${BASH_SOURCE[0]} does not point back at its original location. Use
# SLURM_SUBMIT_DIR (the directory `sbatch` was invoked from) instead.
uv run "${SLURM_SUBMIT_DIR}/bell_state_offload.py" --provider mqss-grpc
