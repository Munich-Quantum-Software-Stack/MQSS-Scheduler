#!/usr/bin/env -S uv run --script
# Copyright (c) 2026 IQM Finland Oy
# All rights reserved.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program. If not, see <https://www.gnu.org/licenses/>.

# /// script
# requires-python = ">=3.11,<3.13"
# dependencies = ["qiskit", "qiskit-aer"]
# ///

"""Bell-state smoke test for batch job submission, using a local Qiskit Aer
simulator instead of a real IQM backend.

Unlike bell_state.py (which requires the iqm-qdmi package and a real,
authenticated Resonance connection via IQM_BASE_URL/IQM_TOKEN), this script
runs entirely locally and needs neither — it exists to validate the Slurm
SPANK job-submission path end to end without needing IQM cloud access.
"""

from __future__ import annotations

from qiskit import QuantumCircuit, transpile
from qiskit_aer import AerSimulator


def main() -> None:
    """Run a Bell state circuit on a local Aer simulator."""
    backend = AerSimulator()

    qc = QuantumCircuit(2)
    qc.h(0)
    qc.cx(0, 1)
    qc.measure_all()

    transpiled_qc = transpile(qc, backend)

    shots = 1000
    job = backend.run(transpiled_qc, shots=shots)
    result = job.result()
    counts = result.get_counts()

    print(f"Counts: {counts}")

    total_shots = sum(counts.values())
    assert total_shots == shots

    # A Bell state should only ever measure '00' or '11'.
    unexpected = set(counts) - {"00", "11"}
    assert not unexpected, f"Unexpected outcomes for a Bell state: {unexpected}"

    print("PASS: Bell state simulated successfully.")


if __name__ == "__main__":
    main()
