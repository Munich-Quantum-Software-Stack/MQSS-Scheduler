"""
example.py


models/
|-- experiments/
|   |-- example/
|       |-- circuits/   QASM circuits   (required)
|       |-- features/   
|       |-- labels/     .pkl labels     (required)



"""

import os
import pickle
import argparse

from qiskit import qasm2
from qiskit import QuantumCircuit

__all__ = ['create_sample_circuit', 'prepare_sample_data']


def create_sample_circuit(num_active_qubits, max_num_qubits):
    """Create a test quantum circuit for a qpu with 20 qubits with h, cx and measurement on the first num_qubits qubits
    """
    circuit = QuantumCircuit(max_num_qubits, num_active_qubits)

    # H gate on first num_qubits qubits
    for i in range(num_active_qubits):
        circuit.h(i)

    # CX gates on first num_qubits - 1 qubits
    for i in range(num_active_qubits - 1):
        circuit.cx(i, i + 1)

    # Measure first num_qubits qubits
    for i in range(num_active_qubits):
        circuit.measure(i, i)

    return circuit


def prepare_sample_data(experiment_name):
    # Prepare directory paths
    current_dir = os.path.dirname(os.path.realpath(__file__))
    experiment_dir = os.path.join(current_dir, "experiments", experiment_name)

    circuits_dir = os.path.join(experiment_dir, "circuits")
    features_dir = os.path.join(experiment_dir, "features")
    labels_dir = os.path.join(experiment_dir, "labels")

    # Create directories
    if not os.path.exists(circuits_dir):
        os.makedirs(circuits_dir)
    if not os.path.exists(features_dir):
        os.makedirs(features_dir)
    if not os.path.exists(labels_dir):
        os.makedirs(labels_dir)

    # Create sample data
    for num_qubits in range(2, 5):
        circ_name = f"{experiment_name}_circuit_{num_qubits}"
        circ_path = os.path.join(circuits_dir, f"{circ_name}.qasm")
        label_path = os.path.join(labels_dir, f"{circ_name}.pkl")

        if not os.path.exists(circ_path):
            # Create and save quantum circuit
            circuit = create_sample_circuit(
                max_num_qubits=20,
                num_active_qubits=num_qubits
            )
            qasm2.dump(circuit, circ_path)

        if not os.path.exists(label_path):
            # Create and save label
            label = num_qubits % 2
            with open(label_path, "wb") as f:
                pickle.dump(label, f)

    return experiment_dir


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Set up an example experiment.")
    parser.add_argument(
        "--experiment_name",
        type=str,
        default="example",
        help="Set experiment name (default: 'example')"
    )
    args = parser.parse_args()
    dir = prepare_sample_data(args.experiment_name)
    print(f"Sample data prepared in: \n {dir}")
