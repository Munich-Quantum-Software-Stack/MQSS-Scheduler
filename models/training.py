import argparse
import os

import pickle
import numpy as np
import networkx as nx

from qiskit import qasm2
from qiskit import QuantumCircuit
from qiskit.converters import circuit_to_dag

from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import GridSearchCV

from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType


def calc_supermarq_plus_features(qc: QuantumCircuit, num_qubits: int) -> tuple:
    """Calculates the Supermarq features for a given quantum circuit. There are three additional features, that cover some issues with the original ones.
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

        # Program communication = circuit's average qubit degree / degree of a complete graph.
        graph = nx.Graph()
        for op in dag.two_qubit_ops():
            q1, q2 = op.qargs
            graph.add_edge(qc.find_bit(q1).index, qc.find_bit(q2).index)
        degree_sum = sum(graph.degree(n) for n in graph.nodes)
        program_communication = degree_sum / \
            (num_qubits * (num_qubits - 1)) if num_qubits > 1 else 0

        # Liveness feature = sum of all entries in the liveness matrix / (num_qubits * depth).
        activity_matrix = np.zeros((qc.num_qubits, dag.depth()))
        for i, layer in enumerate(dag.layers()):
            for op in layer["partition"]:
                for qubit in op:
                    activity_matrix[qc.find_bit(qubit).index, i] = 1
        liveness = (
            np.sum(activity_matrix) / (num_qubits * dag.depth())
            if dag.depth() > 0 and num_qubits > 0
            else 0
        )

        #  Parallelism feature = max((((# of gates / depth) -1) /(# of qubits -1)), 0).
        parallelism = (
            max(((len(dag.gate_nodes()) / dag.depth()) - 1) / (num_qubits - 1), 0)
            if num_qubits > 1 and dag.depth() > 0
            else 0
        )
        # Entanglement-ratio = ratio between # of 2-qubit gates and total number of gates in the circuit.
        entanglement_ratio = len(dag.two_qubit_ops(
        )) / len(dag.gate_nodes()) if len(dag.gate_nodes()) > 0 else 0

        # Critical depth = # of 2-qubit gates along the critical path / total # of 2-qubit gates.
        longest_paths = dag.count_ops_longest_path()
        n_ed = sum(longest_paths[name] for name in {
                   op.name for op in dag.two_qubit_ops()} if name in longest_paths)
        n_e = len(dag.two_qubit_ops())
        critical_depth = n_ed / n_e if n_e != 0 else 0

        # Directed program communication = circuit's average directed qubit degree / degree of a complete directed graph.
        di_graph = nx.DiGraph()
        for op in dag.two_qubit_ops():
            q1, q2 = op.qargs
            di_graph.add_edge(qc.find_bit(q1).index, qc.find_bit(q2).index)
        degree_sum = sum(di_graph.degree(n) for n in di_graph.nodes)
        directed_program_communication = degree_sum / \
            (2 * num_qubits * (num_qubits - 1)) if num_qubits > 1 else 0

        # average number of 1q gates per layer = num of 1-qubit gates in the circuit / depth
        dag.remove_all_ops_named("measure")
        single_qubit_gates_per_layer = (
            (len(dag.gate_nodes()) - len(dag.two_qubit_ops())) /
            dag.depth() if dag.depth() > 0 else 0
        )
        # normalize
        single_qubit_gates_per_layer = (
            single_qubit_gates_per_layer / num_qubits
            if num_qubits > 1
            else 0
        )
        # average number of 2q gates per layer = num of 2-qubit gates in the circuit / depth
        multi_qubit_gates_per_layer = len(
            dag.two_qubit_ops()) / dag.depth() if dag.depth() > 0 else 0
        # normalize
        multi_qubit_gates_per_layer = (
            multi_qubit_gates_per_layer / (num_qubits // 2)
            if num_qubits > 2
            else 0
        )
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
        program_communication,
        critical_depth,
        entanglement_ratio,
        parallelism,
        liveness,
        directed_program_communication,
        single_qubit_gates_per_layer,
        multi_qubit_gates_per_layer,
    )


def create_feature_dict(qc: str | QuantumCircuit, native_gates=[]) -> dict:
    """Creates and returns a feature dictionary for a given quantum circuit.

    Arguments:
        qc: The quantum circuit to be compiled.
        native_gates: The native gates of the quantum computer.

    Returns:
        The feature dictionary of the given quantum circuit.
    """
    ops_list = qc.count_ops()

    # dict_to_featurevector
    res_dct = {gate: 0 for gate in native_gates}
    for key, val in dict(ops_list).items():
        if key in res_dct:
            res_dct[key] = val
    ops_list_dict = res_dct

    feature_dict = {}
    for key in ops_list_dict:
        feature_dict[key] = int(ops_list_dict[key])

    # Create a list of zeros for the one-hot vector
    active_qubits_dict = {qubit: 0 for qubit in qc.qubits}

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


def run(experiment_name: str):
    print(f"Start training for experiment: {experiment_name}")

    # Prepare directory paths
    current_dir = os.path.dirname(os.path.abspath(__file__))
    experiment_dir = os.path.join(current_dir, "experiments", experiment_name)
    circits_dir = os.path.join(experiment_dir, "circuits")
    features_dir = os.path.join(experiment_dir, "features")
    labels_dir = os.path.join(experiment_dir, "labels")

    # Load circuits
    circuits = []
    for file in os.listdir(circits_dir):
        if file.endswith(".qasm"):
            circ_path = os.path.join(circits_dir, file)
            circuits.append(qasm2.load(circ_path))
            # Set circuit name for identification
            circuits[-1].name = file.split(".")[0]

    # Get features
    features = {}
    native_gates = ["h", "cx", "measure"]
    for qc in circuits:
        feat_path = os.path.join(features_dir, f"{qc.name}.pkl")
        # Load feature dictionary if it exists
        if os.path.exists(feat_path):
            with open(feat_path, "rb") as f:
                features[qc.name] = pickle.load(f)
        else:
            # Create and save feature dictionary
            features[qc.name] = create_feature_dict(qc, native_gates)
            with open(feat_path, "wb") as f:
                pickle.dump(features[qc.name], f)

    # Load labels
    labels = {}
    for qc in circuits:
        label_path = os.path.join(labels_dir, f"{qc.name}.pkl")
        # Load labels
        if os.path.exists(label_path):
            with open(label_path, "rb") as f:
                labels[qc.name] = pickle.load(f)
        else:
            print(f"Label file for {qc.name} not found.")
            continue

    # Prepare training data
    X = np.array([list(circ_feat_dict.values())
                  for circ_feat_dict in features.values()], dtype=np.float16)
    y = np.array([circ_label
                  for circ_label in labels.values()], dtype=np.float16)

    # Prepare model setup
    model = RandomForestRegressor(
        criterion="absolute_error",
        random_state=123,
    )

    # Define hyperparameter grid
    if experiment_name == "example":
        grid = {"n_estimators": [50, 100]}
    else:
        grid = {
            "n_estimators": [50, 100],
            "max_depth": [5, None],
            "min_samples_split": [2, 5],
            "min_samples_leaf": [1, 2],
            "min_weight_fraction_leaf": [0.0, 0.1],
            "max_features": ["sqrt", "log2", None],
            "max_leaf_nodes": [None, 10],
            "min_impurity_decrease": [0.0, 0.1],
            "bootstrap": [True, False],
            "oob_score": [False, True],
            "warm_start": [False, True],
            "ccp_alpha": [0.0, 0.1],
            "max_samples": [None, 0.5, 1.0],
        }

    grid_search = GridSearchCV(
        model,
        param_grid=grid,
        # There are only two example circuits
        cv=2 if experiment_name == "example" else 5,
        n_jobs=-1,
        verbose=1,
        error_score='raise',
    )

    # Train model
    grid_search.fit(X, y)

    # Save best model as ONNX file
    model_path = os.path.join(experiment_dir, "rf_reg_model.onnx")
    initial_type = [('float_input', FloatTensorType([None, X.shape[1]]))]
    onnx_model = convert_sklearn(
        grid_search.best_estimator_, initial_types=initial_type)

    with open(model_path, "wb") as f:
        f.write(onnx_model.SerializeToString())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the experiment.")
    parser.add_argument(
        "--experiment_name",
        type=str,
        default="example",
        help="Set experiment name (default: 'example')"
    )
    args = parser.parse_args()
    run(args.experiment_name)
