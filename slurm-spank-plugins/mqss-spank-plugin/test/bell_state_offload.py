#!/usr/bin/env -S uv run --script
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

# /// script
# requires-python = ">=3.11,<3.13"
# dependencies = ["qiskit", "qiskit-aer", "grpcio", "protobuf"]
# ///

"""Bell-state job with a pluggable provider ("adapter"/"offloader").

Two providers:
  --provider aer        Run locally on a Qiskit Aer simulator (same as
                         bell_state_simulator.py) — no network involved.
  --provider mqss-grpc   Serialize the circuit (OpenQASM 3) and offload it via
                         gRPC (see quantum_job.proto) to a remote
                         MQSS-Scheduler-side listener, which executes it and
                         returns real measurement counts.

The mqss-grpc provider targets MQSS_BASE_URL — the same env var
mqss-spank-plugin already injects for every job — rather than a separate,
purpose-built config surface. MQSS_BASE_URL is URL-shaped
(e.g. "http://grpc-server-example:50051"); its host:port (netloc) is used
directly as the gRPC target.
"""

from __future__ import annotations

import argparse
import os
import sys

from qiskit import QuantumCircuit, transpile
from qiskit_aer import AerSimulator

from mqss_grpc_offloader import mqss_grpc_offload, resolve_grpc_target


def build_bell_circuit() -> QuantumCircuit:
    """Build a simple two-qubit Bell state circuit."""
    qc = QuantumCircuit(2)
    qc.h(0)
    qc.cx(0, 1)
    qc.measure_all()
    return qc


def run_local_aer(qc: QuantumCircuit, shots: int) -> dict[str, int]:
    """Execute the circuit locally on a Qiskit Aer simulator."""
    backend = AerSimulator()
    transpiled = transpile(qc, backend)
    job = backend.run(transpiled, shots=shots)
    return dict(job.result().get_counts())


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--provider", choices=("aer", "mqss-grpc"), default="aer")
    parser.add_argument("--shots", type=int, default=1000)
    parser.add_argument(
        "--target",
        default=None,
        help="host:port of the MQSS-Scheduler gRPC listener (mqss-grpc provider "
        "only). Defaults to the netloc of MQSS_BASE_URL.",
    )
    parser.add_argument(
        "--qc-alias",
        default=None,
        help="Target QC alias (mqss-grpc provider only). Defaults to the "
        "MQSS_QC_ALIAS env var (set by mqss-spank-plugin under Slurm), or "
        "empty/unset outside Slurm.",
    )
    args = parser.parse_args()

    qc = build_bell_circuit()

    if args.provider == "aer":
        print("=== Provider: local Aer simulator ===")
        counts = run_local_aer(qc, args.shots)
    else:
        target = args.target
        if not target:
            base_url = os.environ.get("MQSS_BASE_URL")
            if not base_url:
                parser.error(
                    "--target or MQSS_BASE_URL is required for --provider=mqss-grpc"
                )
            target = resolve_grpc_target(base_url)

        print(f"=== Provider: mqss-grpc (offloading to {target}, "
              f"qc_alias={args.qc_alias!r}) ===")
        counts = mqss_grpc_offload(qc, args.shots, target=target,
                                    qc_alias=args.qc_alias)

    if args.provider == "mqss-grpc" and not counts:
        # A real (non-stub) MQSS-Scheduler listener — e.g.
        # examples/grpc-qcscheduler/ — is fire-and-forget: SubmitCircuit
        # returns as soon as the task is queued with the Scheduler, not
        # after execution. Empty counts here means "accepted", not "ran
        # with zero shots" — there is no results round trip yet, so this is
        # as far as this script can verify.
        print("ACCEPTED: circuit queued with the MQSS-Scheduler pipeline "
              "(fire-and-forget listener — no results round trip yet).")
        return

    print(f"Counts: {counts}")

    total_shots = sum(counts.values())
    if total_shots != args.shots:
        print(f"FAIL: expected {args.shots} total shots, got {total_shots}")
        sys.exit(1)

    unexpected = set(counts) - {"00", "11"}
    if unexpected:
        print(f"FAIL: unexpected outcomes for a Bell state: {unexpected}")
        sys.exit(1)

    print("PASS: Bell state executed successfully.")


if __name__ == "__main__":
    main()
