"""test_training.py.

This script tests the functions in src/training.py.
"""

from __future__ import annotations

import pickle
import shutil

import numpy as np
import onnxruntime as ort

from predictor.data_preparation import create_sample_circuit, prepare_sample_data
from predictor.training import calc_supermarq_plus_features, create_feature_dict, train_model


def test_calc_supermarq_plus_features() -> None:
    """Test the calculation of the supermarq+ features."""
    # Create a test quantum circuit
    num_qubits = 2
    circuit = create_sample_circuit(max_num_qubits=20, num_active_qubits=num_qubits)

    # Calculate the features
    features = calc_supermarq_plus_features(circuit, num_qubits)

    # Check the features
    for feat in features:
        assert feat >= 0.0
        assert feat <= 1.0


def test_create_feature_dict() -> None:
    """Test the creation of the feature dictionary."""
    # Create a test quantum circuit
    num_qubits = 2
    circuit = create_sample_circuit(max_num_qubits=20, num_active_qubits=num_qubits)

    # Create the feature dictionary
    feature_dict = create_feature_dict(circuit, native_gates=["h", "cx"])

    # Define the expected features
    expected_feature_dict = {
        "h": 2,
        "cx": 1,
        "Qubit(QuantumRegister(20, 'q'), 0)": 1,
        "Qubit(QuantumRegister(20, 'q'), 1)": 1,
        "Qubit(QuantumRegister(20, 'q'), 2)": 0,
        "Qubit(QuantumRegister(20, 'q'), 3)": 0,
        "Qubit(QuantumRegister(20, 'q'), 4)": 0,
        "Qubit(QuantumRegister(20, 'q'), 5)": 0,
        "Qubit(QuantumRegister(20, 'q'), 6)": 0,
        "Qubit(QuantumRegister(20, 'q'), 7)": 0,
        "Qubit(QuantumRegister(20, 'q'), 8)": 0,
        "Qubit(QuantumRegister(20, 'q'), 9)": 0,
        "Qubit(QuantumRegister(20, 'q'), 10)": 0,
        "Qubit(QuantumRegister(20, 'q'), 11)": 0,
        "Qubit(QuantumRegister(20, 'q'), 12)": 0,
        "Qubit(QuantumRegister(20, 'q'), 13)": 0,
        "Qubit(QuantumRegister(20, 'q'), 14)": 0,
        "Qubit(QuantumRegister(20, 'q'), 15)": 0,
        "Qubit(QuantumRegister(20, 'q'), 16)": 0,
        "Qubit(QuantumRegister(20, 'q'), 17)": 0,
        "Qubit(QuantumRegister(20, 'q'), 18)": 0,
        "Qubit(QuantumRegister(20, 'q'), 19)": 0,
        "depth": 3.0,
        "num_qubits": 2.0,
        "program_communication": 1.0,
        "critical_depth": 1.0,
        "entanglement_ratio": 1 / 3,
        "parallelism": 0.0,
        "liveness": 1.0,
        "directed_program_communication": 0.5,
        "single_qubit_gates_per_layer": 0.5,
        "multi_qubit_gates_per_layer": 0.5,
    }

    # Check the feature dictionary
    for exp_key, exp_value, key, value in zip(
        expected_feature_dict.keys(),
        expected_feature_dict.values(),
        feature_dict.keys(),
        feature_dict.values(),
        strict=False,
    ):
        assert exp_key == str(key)
        assert exp_value == value


def test_train_model() -> None:
    """Test the training and inference of the model."""
    # Define the experiment name
    experiment_name = "test"

    # Prepare the example data
    experiment_dir = prepare_sample_data(experiment_name)
    features_dir = experiment_dir / "features"
    labels_dir = experiment_dir / "labels"

    # Run the training
    train_model(experiment_name)

    # Load the trained ONNX model
    model_path = experiment_dir / f"{experiment_name}.onnx"
    session = ort.InferenceSession(model_path)

    # Load the feature vectors and labels
    feature_files = list(features_dir.glob("*.pkl"))
    label_files = list(labels_dir.glob("*.pkl"))

    # Load a feature vector and label for the test circuit
    with feature_files[0].open("rb") as f:
        feats_data = pickle.load(f)
    with label_files[0].open("rb") as f:
        label_data = pickle.load(f)

    feats = np.array(list(feats_data.values()), dtype=np.float32)
    feats = feats.reshape(1, -1)  # because only single feature vector
    label = np.array(label_data, dtype=np.float32)

    # Run inference
    input_name = session.get_inputs()[0].name
    result = session.run(None, {input_name: feats})

    # Check the results
    assert isinstance(result[0], type(label))

    # Delete the experiment directory
    shutil.rmtree(experiment_dir)
