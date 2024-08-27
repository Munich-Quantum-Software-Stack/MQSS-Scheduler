"""test_utils.py.

This script tests the functions in src/utils.py.
"""

from __future__ import annotations

import os
import pickle

from qiskit import QuantumCircuit, qasm2

# ruff: noqa: PLC2701
from predictor.utils import create_sample_circuit, prepare_sample_data


def test_create_sample_circuit() -> None:
    """Test the creation of a sample quantum circuit."""
    num_active_qubits = 3
    max_num_qubits = 5

    # Create the sample circuit
    circuit = create_sample_circuit(num_active_qubits, max_num_qubits)

    # Check the circuit properties
    assert isinstance(circuit, QuantumCircuit)
    assert circuit.num_qubits == max_num_qubits
    assert circuit.num_clbits == num_active_qubits

    # Check the circuit structure
    assert circuit.data[0].operation.name == "h"
    assert circuit.data[1].operation.name == "h"
    assert circuit.data[2].operation.name == "h"

    assert circuit.data[3].operation.name == "cx"
    assert circuit.data[4].operation.name == "cx"

    assert circuit.data[5].operation.name == "measure"
    assert circuit.data[6].operation.name == "measure"
    assert circuit.data[7].operation.name == "measure"


def test_prepare_sample_data() -> None:
    """Test the preparation of sample data."""
    experiment_name = "test_experiment"

    # Prepare the sample data
    experiment_dir = prepare_sample_data(experiment_name)

    # Check the directory structure
    assert (experiment_dir / "circuits").exists()
    assert (experiment_dir / "features").exists()
    assert (experiment_dir / "labels").exists()

    # Check the created files
    for num_qubits in range(2, 5):
        circ_name = f"{experiment_name}_circuit_{num_qubits}"
        circ_path = experiment_dir / "circuits" / f"{circ_name}.qasm"
        label_path = experiment_dir / "labels" / f"{circ_name}.pkl"

        assert circ_path.exists()
        assert label_path.exists()

        # Load and check the quantum circuit
        circuit = qasm2.load(circ_path)
        assert isinstance(circuit, QuantumCircuit)
        assert circuit.num_qubits == 20
        assert circuit.num_clbits == num_qubits

        # Load and check the label
        with label_path.open("rb") as f:
            label = pickle.load(f)
        assert label == num_qubits % 2

    # Clean up the created directories
    os.system(f"rm -rf {experiment_dir}")
