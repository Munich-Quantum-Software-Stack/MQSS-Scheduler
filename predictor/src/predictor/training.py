"""training.py.

This module provides functions to calculate circuit feature dictionaries and
a training routine to train a RandomForestRegressor model on the extracted features.

Functions:
- calc_supermarq_plus_features(qc: QuantumCircuit, num_qubits: int) -> tuple:
    Calculates the Supermarq features for a given quantum circuit.
- create_feature_dict(circuit: QuantumCircuit, native_gates: list) -> dict:
    Creates a dictionary of features for a given quantum circuit.
- train_model(experiment_name: str):
    Runs the training process for the given experiment.
- run_training():
    Python entry point to train a model on the experiment data.
"""

from __future__ import annotations

import argparse
import pickle
from pathlib import Path

import networkx as nx
import numpy as np
from qiskit import QuantumCircuit, qasm2
from qiskit.converters import circuit_to_dag
from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import make_scorer
from sklearn.model_selection import GridSearchCV

__all__ = ["calc_supermarq_plus_features", "create_feature_dict", "train_model"]


def calc_supermarq_plus_features(
    qc: QuantumCircuit, num_qubits: int
) -> tuple[float, float, float, float, float, float, float, float]:
    """Calculates the Supermarq features for a given quantum circuit.

    There are three additional features, that cover some issues with the original ones.
    Code adapted from https://github.com/Infleqtion/client-superstaq/blob/91d947f8cc1d99f90dca58df5248d9016e4a5345/supermarq-benchmarks/supermarq/converters.py.

    Arguments:
        qc: The quantum circuit to be compiled.
        num_qubits: The number of qubits in the quantum circuit.

    Returns:
        The Supermarq features for the given quantum circuit.
    """
    try:
        dag = circuit_to_dag(qc)
        dag.remove_all_ops_named("barrier")
        depth = dag.depth()  # including measurements

        # Program communication = circuit's average qubit degree / degree of a complete graph.
        graph = nx.Graph()
        for op in dag.two_qubit_ops():
            q1, q2 = op.qargs
            graph.add_edge(qc.find_bit(q1).index, qc.find_bit(q2).index)
        degree_sum = sum(graph.degree(n) for n in graph.nodes)
        program_communication = degree_sum / (num_qubits * (num_qubits - 1)) if num_qubits > 1 else 0

        # Liveness feature = sum of all entries in the liveness matrix / (num_qubits * depth).
        activity_matrix = np.zeros((qc.num_qubits, depth))
        for i, layer in enumerate(dag.layers()):
            for op in layer["partition"]:
                for qubit in op:
                    activity_matrix[qc.find_bit(qubit).index, i] = 1
        liveness = np.sum(activity_matrix) / (num_qubits * depth) if depth > 0 and num_qubits > 0 else 0

        #  Parallelism feature = max((((# of gates / depth) -1) /(# of qubits -1)), 0).
        parallelism = (
            max(((len(dag.gate_nodes()) / depth) - 1) / (num_qubits - 1), 0) if num_qubits > 1 and depth > 0 else 0
        )
        # Entanglement-ratio = ratio between # of 2-qubit gates and total number of gates in the circuit.
        entanglement_ratio = len(dag.two_qubit_ops()) / len(dag.gate_nodes()) if len(dag.gate_nodes()) > 0 else 0

        # Critical depth = # of 2-qubit gates along the critical path / total # of 2-qubit gates.
        longest_paths = dag.count_ops_longest_path()
        n_ed = sum(longest_paths[name] for name in {op.name for op in dag.two_qubit_ops()} if name in longest_paths)
        n_e = len(dag.two_qubit_ops())
        critical_depth = n_ed / n_e if n_e != 0 else 0

        # Directed program communication = circuit's average directed qubit degree / degree of a complete directed graph.
        di_graph = nx.DiGraph()
        for op in dag.two_qubit_ops():
            q1, q2 = op.qargs
            di_graph.add_edge(qc.find_bit(q1).index, qc.find_bit(q2).index)
        degree_sum = sum(di_graph.degree(n) for n in di_graph.nodes)
        directed_program_communication = degree_sum / (2 * num_qubits * (num_qubits - 1)) if num_qubits > 1 else 0

        # average number of 1q gates per layer = num of 1-qubit gates in the circuit / (depth wo measurements)
        # HACK: dag.remove_all_ops_named("measure") is broken
        depth_wo_meas = depth - 1
        single_qubit_gates_per_layer = (
            (len(dag.gate_nodes()) - len(dag.two_qubit_ops())) / depth_wo_meas if depth_wo_meas > 0 else 0
        )
        # normalize
        single_qubit_gates_per_layer = single_qubit_gates_per_layer / num_qubits if num_qubits > 0 else 0
        # average number of 2q gates per layer = num of 2-qubit gates in the circuit / depth
        multi_qubit_gates_per_layer = len(dag.two_qubit_ops()) / depth_wo_meas if depth_wo_meas > 0 else 0
        # normalize
        multi_qubit_gates_per_layer = multi_qubit_gates_per_layer / (num_qubits // 2) if num_qubits > 1 else 0
    except Exception as e:
        print(f"Error calculating Supermarq features: {e}")

    assert 0 <= program_communication <= 1
    assert 0 <= critical_depth <= 1
    assert 0 <= entanglement_ratio <= 1
    assert 0 <= parallelism <= 1
    assert 0 <= liveness <= 1

    assert 0 <= directed_program_communication <= 1
    assert 0 <= single_qubit_gates_per_layer <= 1
    assert 0 <= multi_qubit_gates_per_layer <= 1

    return (
        float(program_communication),
        float(critical_depth),
        float(entanglement_ratio),
        float(parallelism),
        float(liveness),
        float(directed_program_communication),
        float(single_qubit_gates_per_layer),
        float(multi_qubit_gates_per_layer),
    )


def create_feature_dict(qc: QuantumCircuit, native_gates: list[str] | None = None) -> dict[str, float]:
    """Creates and returns a feature dictionary for a given quantum circuit.

    Arguments:
        qc: The quantum circuit to be compiled.
        native_gates: The native gates of the quantum computer.

    Returns:
        The feature dictionary of the given quantum circuit.
    """
    if native_gates is None:
        native_gates = []
    ops_list = qc.count_ops()

    # dict_to_featurevector
    res_dct = dict.fromkeys(native_gates, 0)
    for key, val in dict(ops_list).items():
        if key in res_dct:
            res_dct[key] = val
    ops_list_dict = res_dct

    feature_dict: dict[str, float] = {}
    for key in ops_list_dict:
        feature_dict[key] = int(ops_list_dict[key])

    # Create a list of zeros for the one-hot vector
    active_qubits_dict = dict.fromkeys(qc.qubits, 0)

    # Iterate over the operations in the quantum circuit
    for op in qc.data:
        if op.operation.name in ["barrier", "measure"]:
            continue
        # Mark the qubits that are used in the operation as active
        for qubit in op.qubits:
            active_qubits_dict[qubit] = 1

    # Add the active qubits to the feature dictionary
    feature_dict.update(active_qubits_dict)
    num_qubits = sum(active_qubits_dict.values())

    feature_dict["depth"] = float(qc.depth())
    feature_dict["num_qubits"] = float(num_qubits)

    supermarq_features = calc_supermarq_plus_features(qc, num_qubits)

    feature_dict["program_communication"] = supermarq_features[0]
    feature_dict["critical_depth"] = supermarq_features[1]
    feature_dict["entanglement_ratio"] = supermarq_features[2]
    feature_dict["parallelism"] = supermarq_features[3]
    feature_dict["liveness"] = supermarq_features[4]

    feature_dict["directed_program_communication"] = supermarq_features[5]
    feature_dict["single_qubit_gates_per_layer"] = supermarq_features[6]
    feature_dict["multi_qubit_gates_per_layer"] = supermarq_features[7]

    return feature_dict


def train_model(experiment_name: str) -> Path:
    """Runs the training process for the given experiment.

    This function loads the circuits and the corresponding labels, generates the features,
    and trains a RandomForestRegressor model on the extracted features.

    Arguments:
        experiment_name: The name of the experiment.

    Returns:
        The path to the trained model.
    """
    print(f"Start training for experiment: {experiment_name}")

    # Prepare directory paths
    root_dir = Path(__file__).resolve().parents[2]
    experiment_dir = root_dir / "data" / experiment_name

    circuits_dir = experiment_dir / "circuits"
    features_dir = experiment_dir / "features"
    labels_dir = experiment_dir / "labels"

    # Load circuits
    print("Loading circuits...")
    circuits = []
    for circ_path in circuits_dir.glob("*.qasm"):
        circuits.append(qasm2.load(circ_path))
        # Set circuit name for identification
        circuits[-1].name = circ_path.stem

    # Get features
    print("Extracting features...")
    features = {}
    native_gates = ["h", "cx", "measure"]
    for qc in circuits:
        feat_path = features_dir / f"{qc.name}.pkl"
        # Load feature dictionary if it exists
        if feat_path.exists():
            with feat_path.open("rb") as f:
                features[qc.name] = pickle.load(f)
        else:
            # Create and save feature dictionary
            features[qc.name] = create_feature_dict(qc, native_gates)
            with feat_path.open("wb") as f:
                pickle.dump(features[qc.name], f)

    # Load labels
    print("Loading labels...")
    labels = {}
    for qc in circuits:
        label_path = labels_dir / f"{qc.name}.pkl"
        # Load labels
        if label_path.exists():
            with label_path.open("rb") as f:
                labels[qc.name] = pickle.load(f)
        else:
            print(f"Label file for {qc.name} not found.")
            features.pop(qc.name, None)
            continue

    # Prepare training data
    x = np.array([list(circ_feat_dict.values()) for circ_feat_dict in features.values()], dtype=np.float32)
    y = np.array(list(labels.values()), dtype=np.float32)

    assert x.shape[0] == y.shape[0], "Number of features and labels do not match."

    # Prepare model setup
    model = RandomForestRegressor(random_state=123)

    # Define hyperparameter grid search
    if experiment_name == "example" or experiment_name == "test":
        grid = {"n_estimators": [50, 100]}  # Speed up the sample training
        kwargs = {
            "cv": 2,  # Only necessary to work with the small sample dataset
            "scoring": make_scorer(lambda y, y_pred: np.mean(y - y_pred)),
        }
    else:
        # Full hyperparameter grid search for real experiments
        kwargs = {"n_jobs": -1, "verbose": 1, "cv": 5}
        grid = {  # NOTE: Can (and should) be adjusted for comprehensive search
            "n_estimators": [20, 40, 80, 160],
            "max_depth": [5, 10, 20, 40],
            "min_samples_split": [2, 5],
            "min_samples_leaf": [1, 2],
            "min_weight_fraction_leaf": [0.0, 0.1],  # type: ignore[list-item]
            "max_features": ["sqrt", "log2", None],  # type: ignore[list-item]
            "max_leaf_nodes": [None, 10],  # type: ignore[list-item]
            "min_impurity_decrease": [0.0, 0.1],  # type: ignore[list-item]
            "bootstrap": [True, False],
            "oob_score": [False, True],
            "warm_start": [False, True],
            "ccp_alpha": [0.0, 0.1],  # type: ignore[list-item]
            "max_samples": [None, 0.5, 1.0],  # type: ignore[list-item]
            "criterion": ["absolute_error"],  # type: ignore[list-item]
        }

    grid_search = GridSearchCV(model, param_grid=grid, error_score="raise", **kwargs)

    # Train model
    print("Training model...")
    grid_search.fit(x, y)

    # Save best model as ONNX file
    model_path = experiment_dir / f"{experiment_name}.onnx"
    initial_type = [("float_input", FloatTensorType([None, x.shape[1]]))]
    onnx_model = convert_sklearn(grid_search.best_estimator_, initial_types=initial_type)

    with model_path.open("wb") as f:
        f.write(onnx_model.SerializeToString())

    print(f"Model trained and saved as ONNX file: \n {model_path}")
    return model_path


def run_training() -> None:
    """Python entry point to train a model on the experiment data."""
    parser = argparse.ArgumentParser(description="Train a model on the experiment data.")
    parser.add_argument(
        "--experiment-name", type=str, default="example", help="Set experiment name (default: 'example')"
    )
    args = parser.parse_args()
    train_model(args.experiment_name)
