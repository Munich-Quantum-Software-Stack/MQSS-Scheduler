import os
import pytest
import pickle
from qiskit import QuantumCircuit, qasm2
from models.training import calc_supermarq_plus_features, create_feature_dict, run


def create_test_circuit(num_qubits):
    circuit = QuantumCircuit(num_qubits)

    for i in range(num_qubits):
        circuit.h(i)

    for i in range(num_qubits - 1):
        circuit.cx(i, i + 1)

    circuit.measure_all()

    return circuit


def test_calc_supermarq_plus_features():
    num_qubits = 2

    # Create a test quantum circuit
    circuit = create_test_circuit(num_qubits)

    # Calculate the features
    features = calc_supermarq_plus_features(circuit, num_qubits)

    # Check the features
    for feat in features:
        assert feat >= 0.0
        assert feat <= 1.0


def test_create_feature_dict():
    num_qubits = 2

    # Create a test quantum circuit
    circuit = create_test_circuit(num_qubits)

    # Create the feature dictionary
    feature_dict = create_feature_dict(circuit, native_gates=['h', 'cx'])

    # Check the feature dictionary
    for key, value in feature_dict.items():
        assert isinstance(key, str)
        if isinstance(value, float):
            assert value >= 0.0
            assert value <= 1.0
        else:
            assert isinstance(value, int)


def test_run():
    # Define the experiment name
    experiment_name = "test"

    # Prepare directory paths
    parent_dir = os.path.dirname(os.path.abspath(__file__)).split("tests")[0]
    experiment_dir = os.path.join(
        parent_dir, "models", "experiments", experiment_name
    )

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

    # Create test data
    for num_qubits in range(2, 5):
        circ_name = f"test_circuit_{num_qubits}"
        circ_path = os.path.join(circuits_dir, f"{circ_name}.qasm")
        label_path = os.path.join(labels_dir, f"{circ_name}.pkl")

        if not os.path.exists(circ_path):
            # Create and save quantum circuit
            circuit = create_test_circuit(num_qubits)
            qasm2.dump(circuit, circ_path)

        if not os.path.exists(label_path):
            # Create and save label
            label = num_qubits % 2
            with open(label_path, "wb") as f:
                pickle.dump(label, f)

    # Run the training
    run(experiment_name)

    # Delete the experiment directory
    os.system(f"rm -rf {experiment_dir}")
