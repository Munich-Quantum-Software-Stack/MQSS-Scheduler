"""data_preparation.py.

This module provides functions to prepare some test and example data for the training.py script.

Functions:
- create_sample_circuit: Create a sample quantum circuit
- prepare_sample_data: Prepare sample data for an experiment
- run_data_preparation: Python entry point to prepare sample data for the training
"""

from __future__ import annotations

import argparse
import pickle
from pathlib import Path

from qiskit import QuantumCircuit, qasm2

__all__ = ["create_sample_circuit", "prepare_sample_data"]


def create_sample_circuit(num_active_qubits: int, max_num_qubits: int) -> QuantumCircuit:
    """Create a test quantum circuit for an IQM device with max_num_qubits qubits.

    Returns circuit with "r", "cz" and "measurement" operations on the first num_qubits qubits.
    """
    circuit = QuantumCircuit(max_num_qubits, num_active_qubits)

    # R gate on first num_qubits qubits
    for i in range(num_active_qubits):
        circuit.r(1.0, 1.0, i)

    # CZ gates on first num_qubits - 1 qubits
    for i in range(num_active_qubits - 1):
        circuit.cz(i, i + 1)

    # Measure first num_qubits qubits
    for i in range(num_active_qubits):
        circuit.measure(i, i)

    return circuit


def prepare_sample_data(experiment_name: str) -> Path:
    """Prepare sample data for the ML training.

    The data will be saved in the experiments/experiment_name directory.
    """
    # Prepare directory paths
    root_dir = Path(__file__).resolve().parents[2]
    experiment_dir = root_dir / "data" / experiment_name

    circuits_dir = experiment_dir / "circuits"
    features_dir = experiment_dir / "features"
    labels_dir = experiment_dir / "labels"

    # Create directories
    circuits_dir.mkdir(parents=True, exist_ok=True)
    features_dir.mkdir(parents=True, exist_ok=True)
    labels_dir.mkdir(parents=True, exist_ok=True)

    # Create sample data
    for num_qubits in range(2, 5):
        circ_name = f"{experiment_name}_circuit_{num_qubits}"
        circ_path = circuits_dir / f"{circ_name}.qasm"
        label_path = labels_dir / f"{circ_name}.pkl"

        # Create and save quantum circuit
        circuit = create_sample_circuit(max_num_qubits=20, num_active_qubits=num_qubits)
        qasm2.dump(circuit, circ_path)

        # Create and save label
        label = num_qubits % 2
        with label_path.open("wb") as f:
            pickle.dump(label, f)

    return experiment_dir


def run_data_preparation() -> None:
    """Python entry point to prepare sample data for the training."""
    parser = argparse.ArgumentParser(description="Set up an example experiment.")
    parser.add_argument(
        "--experiment-name", type=str, default="example", help="Set experiment name (default: 'example')"
    )
    args = parser.parse_args()
    directory = prepare_sample_data(args.experiment_name)
    print(f"Sample data prepared in: \n{directory}")
