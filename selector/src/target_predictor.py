import os
import pickle
import random
import numpy as np

import networkx as nx
import onnxruntime as ort
import matplotlib.pyplot as plt

from pathlib import Path

from sklearn.ensemble import RandomForestRegressor, RandomForestClassifier
from sklearn.model_selection import GridSearchCV, StratifiedKFold
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import classification_report, roc_auc_score, RocCurveDisplay

from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType

from qiskit import QuantumCircuit
from qiskit.converters import circuit_to_dag


# -------------------------------------------------
# Util functions
# -------------------------------------------------
def split_data(data: list, split_factor: float) -> tuple[list, list]:
    """!
    @brief Splits a dataset into a training and test set for model training.
    
    This function takes a dataset and splits it into two parts: one for training and 
    the other for testing, based on the specified split factor.
    
    @param data A list of data to be split.
    @param split_factor A float representing the proportion of data to be used for training (between 0 and 1).
    @return A tuple containing two lists:
        - The first list contains the training data.
        - The second list contains the test data.
    """

    # Shuffle data so that each shuffle is slightly diffrent
    random.shuffle(data)
    # Calculate the split point
    split_index = len(data) * 2 // 3
    # Split the list into two parts
    list1 = data[:split_index]  # first 2/3
    list2 = data[split_index:]  # the rest 1/3

    return list1, list2

def check_gates_in_circuit(qc: QuantumCircuit):
    """!
    @brief Checks for the presence of specific gates in a quantum circuit.

    This function decomposes a given quantum circuit into its individual operations 
    and checks whether certain gates (such as 'sxdg', 'z', 'h', 's', 'x', 'y', 'sdg', 'cx', 'swap'.) are present in the circuit.
    IMPORTANT: Decompose is not stable so if errors occur make sure to check that first.
    @param qc The quantum circuit to be checked for gates. It is expected to be a 
              `QuantumCircuit` object that may contain various gates.

    @return A dictionary where the keys are the names of the gates (e.g., 'sxdg', 'z', 'h', etc.) 
            and the values are booleans indicating whether each gate is present in the circuit (True) 
            or not (False).
    """

    # Decompose the circuit to individual operations (if any)
    qc = qc.decompose()
    gate_dict = {'sxdg': False, 'z': False, 'h': False, 's': False, 'x': False, 
                 'y': False, 'sdg': False, 'cx': False, 'swap': False}
    for instruction in qc.data:
        gate_name = instruction.name
        # check if the gate name is in the dictionary
        if gate_name in gate_dict:
            gate_dict[gate_name] = True

    return gate_dict

def calc_supermarq_plus_features(qc: QuantumCircuit, num_qubits: int):
    """!
    @brief Calculates the Supermarq features and additional features for a given quantum circuit.

    This function computes the Supermarq features as well as three additional features designed to address 
    certain limitations of the original Supermarq features. These features provide insights into the quantum 
    circuit's structure and behavior. The implementation is adapted from:
    https://github.com/Infleqtion/client-superstaq/blob/91d947f8cc1d99f90dca58df5248d9016e4a5345/supermarq-benchmarks/supermarq/converters.py.

    @param qc: The quantum circuit to be analyzed.
    @param num_qubits: The number of qubits in the quantum circuit.
    @return A tuple containing the Supermarq features and additional features for the quantum circuit. The 
            features are as follows:
            - Program communication
            - Critical depth
            - Entanglement ratio
            - Parallelism
            - Liveness
            - Directed program communication
            - Average number of 1-qubit gates
            - Average number of 2-qubit gates
    """
    try:
        critical_depth = 1.0
        qc = qc.decompose()
        dag = circuit_to_dag(qc)
        dag.remove_all_ops_named("barrier")
        dag.remove_all_ops_named("measure")
        depth = dag.depth()

        # Program communication = 
        #   circuit's average qubit degree / degree of a complete interaction graph
        if len(dag.two_qubit_ops()) > 0:
            graph = nx.Graph()
            for op in dag.two_qubit_ops():
                q1, q2 = op.qargs
                graph.add_edge(qc.find_bit(q1).index, qc.find_bit(q2).index)
            degree_sum = sum(graph.degree(n) for n in graph.nodes)
        else:
            degree_sum = 0.0

        # Degree of a complete graph
        max_degree = num_qubits * (num_qubits - 1)
        program_communication = degree_sum / max_degree if max_degree > 0 else 0

        # NOTE: layers() does change the DAG object
        # Liveness feature = 
        #   sum of all entries in the liveness matrix / (num_qubits * depth)
        activity_matrix = np.zeros((num_qubits, depth))
        for i, layer in enumerate(dag.layers()):
            for op in layer["partition"]:
                for qubit in op:
                    activity_matrix[qc.find_bit(qubit).index, i] = 1
        max_activity = num_qubits * depth
        liveness = np.sum(activity_matrix) / max_activity if max_activity > 0 else 0

        # Parallelism feature = 
        #   ((num of gates / depth) - 1) / (num of qubits - 1)).
        num_gates = len(dag.gate_nodes())
        parallelism = ((num_gates / depth) - 1) / (num_qubits - 1) if num_qubits > 1 and depth > 0 else 0

        # Entanglement-ratio = 
        #   ratio between # of 2-qubit gates and total number of gates in the circuit
        if len(dag.two_qubit_ops())>0:
            entanglement_ratio = len(dag.two_qubit_ops()) / num_gates if num_gates > 0 else 0
        else:
            entanglement_ratio = 0.0
        
        # Critical depth = 
        #   Num. 2-qubit gates along the critical path / total num. 2-qubit gates.
        if (num_qubits) > 1:
            ops_on_crit_path = dag.count_ops_longest_path()
            n_ed = sum(ops_on_crit_path[name] for name in {op.name for op in dag.two_qubit_ops()} if name in ops_on_crit_path)
            n_e = len(dag.two_qubit_ops())
            critical_depth = n_ed / n_e if n_e != 0 else 0
        else:
            critical_depth = 1.0

        # Directed program communication = 
        #   circuit's average directed qubit degree / degree of a complete directed graph.
        if(len(dag.two_qubit_ops())) > 0:
            di_graph = nx.DiGraph()
            for op in dag.two_qubit_ops():
                q1, q2 = op.qargs
                di_graph.add_edge(qc.find_bit(q1).index, qc.find_bit(q2).index)
            degree_sum = sum(di_graph.degree(n) for n in di_graph.nodes)
        else:
            degree_sum = 0.0
        directed_program_communication = degree_sum / (2 * num_qubits * (num_qubits - 1)) if num_qubits > 1 else 0

        # Average num. 1-qubit gates = 
        #   num of 1-qubit gates in the circuit / (num qubits * depth)
        single_qubit_gates = (
            (len(dag.gate_nodes()) - len(dag.two_qubit_ops())) / (num_qubits * depth)
            if depth > 0 and num_qubits > 0
            else 0
        )

        # Average num. 2-qubit gates = 
        #   num of 2-qubit gates in the circuit / (num qubits * depth)
        multi_qubit_gates = (
            len(dag.two_qubit_ops()) / ((num_qubits // 2) * depth) 
            if depth > 0 and num_qubits > 1 else 0
        )

    except Exception as e:
        print(f"Error calculating Supermarq features: {e}")

    assert 0 <= program_communication <= 1
    assert 0 <= critical_depth <= 1
    assert 0 <= entanglement_ratio <= 1
    assert 0 <= parallelism <= 1
    assert 0 <= liveness <= 1
    assert 0 <= directed_program_communication <= 1
    assert 0 <= single_qubit_gates <= 1
    assert 0 <= multi_qubit_gates <= 1

    return (float(program_communication), float(critical_depth),
        float(entanglement_ratio), float(parallelism),
        float(liveness), float(directed_program_communication),
        float(single_qubit_gates), float(multi_qubit_gates))

def create_feature_dict(qc: QuantumCircuit) -> dict[str, float]:
    """!
    @brief Creates and returns a feature dictionary for a given quantum circuit.

    This function analyzes a quantum circuit and creates a feature dictionary containing various 
    characteristics of the circuit, such as depth, number of qubits, and Supermarq features (e.g., 
    program communication, critical depth, entanglement ratio). It uses the `calc_supermarq_plus_features` 
    function to compute additional features specific to the quantum circuit.

    @param qc The quantum circuit to be analyzed.
    @return A dictionary containing the features of the quantum circuit. The features include:
            - analysis of the occuring gates check check_gates_in_circuits for which gates
            - "depth" (float): The depth of the quantum circuit.
            - "num_qubits" (float): The number of qubits in the quantum circuit.
            - "program_communication" (float): The program communication feature of the circuit.
            - "critical_depth" (float): The critical depth of the circuit.
            - "entanglement_ratio" (float): The entanglement ratio of the circuit.
            - "parallelism" (float): The parallelism feature of the circuit.
            - "liveness" (float): The liveness feature of the circuit.
            - "directed_program_communication" (float): The directed program communication feature.
            - "single_qubit_gates" (float): The number of single-qubit gates used.
            - "multi_qubit_gates" (float): The number of multi-qubit gates used.
    """

    # NOTE: This function currently works with the IQM native gate set ["r", "cz"].
    # It should be adjusted to all possible gates the model is supposed to work with.
    # native_gates = ["r", "cz"]

    # create a dictionary to store the features
    feature_dict: dict[str, float] = {}

    # create a list of zeros for the one-hot vector
    active_qubits_dict = dict.fromkeys(qc.qubits, 0)

    # get the operations in the circuit
    qc_ops = qc.count_ops()
    print("Num. operations: ", qc_ops)

    # Iterate over the operations in the quantum circuit
    for op in qc.data:
        if op.operation.name in ["barrier", "measure"]:
            continue
        # Mark the qubits that are used in the operation as active
        for qubit in op.qubits:
            active_qubits_dict[qubit] = 1

    # Add the active qubits to the feature dictionary
    num_qubits = qc.num_qubits

    # Trial start
    updated_gate_dict = check_gates_in_circuit(qc)
    feature_dict.update(updated_gate_dict)
    # Trial end
    
    # Extract the Supermarq features
    supermarq_features = calc_supermarq_plus_features(qc, num_qubits)

    feature_dict["depth"] = float(qc.depth())
    feature_dict["num_qubits"] = float(num_qubits)
    feature_dict["program_communication"] = supermarq_features[0]
    feature_dict["critical_depth"] = supermarq_features[1]
    feature_dict["entanglement_ratio"] = supermarq_features[2]
    feature_dict["parallelism"] = supermarq_features[3]
    feature_dict["liveness"] = supermarq_features[4]
    feature_dict["directed_program_communication"] = supermarq_features[5]
    feature_dict["single_qubit_gates"] = supermarq_features[6]
    feature_dict["multi_qubit_gates"] = supermarq_features[7]

    return feature_dict

# -------------------------------------------------
# Model training
# -------------------------------------------------
def train_randomforest_model(model_name: str, circuits_path: str, label_path: str):
    """!
    @brief Runs the training process for the given experiment.

    This function performs the complete training process for the experiment by:
    - Loading the quantum circuits and the corresponding labels.
    - Generating features for the circuits.
    - the circuits are matched over their 'names'
    - Training a RandomForestRegressor model on the extracted features.

    @param experiment_name The name of the experiment to train the model on.
    @param path to the circuits file 
    @param path to the lable file
    @return A string representing the path to the trained model.
    """

    # Set random seed for reproducibility
    random.seed(42)

    # Prepare the datasets for training and testing
    with open(circuits_path, 'rb') as f:
        circuits = pickle.load(f)
    print("-----------------------------------------")
    print("Total num. circuits: ", len(circuits))

    train_set, test_set = split_data(circuits, 0.7)
    with open('../data/training_set.pkl', 'wb') as f:
        pickle.dump(train_set, f)
    with open('../data/test_set.pkl', 'wb') as f:
        pickle.dump(test_set, f)
    print("  + num. train circuits: ", len(train_set))
    print("  + num. test circuits: ", len(test_set))

    # Load the circuits for training
    print("-----------------------------------------")
    print("Loading circuits for training ...")
    train_circuits = []
    for name, circ in train_set:
        train_circuits.append(circ.decompose())
        # set circuit name for identification as an additional attribute
        train_circuits[-1].name = name
    # Double check the information
    # print("Len(train_circuits): ", len(train_circuits))
    # print("  + 1st_element: ", train_circuits[0])
    # print("  + att_name: ", train_circuits[0].name)

    # Extract the features
    print("-----------------------------------------")
    print("Extracting features for the train set ...")
    features_train_dir = Path(f"../data/features/train")
    features_train = {}
    for qcirc in train_circuits:
        feature_train_filepath = features_train_dir / f"{qcirc.name}.pkl"
        # Load feature dictionary if it exists
        if feature_train_filepath.exists():
            with feature_train_filepath.open("rb") as f:
                features_train[qcirc.name] = pickle.load(f)
        # Otherwise, create and save feature dictionary
        else:
            feature_train_filepath.parent.mkdir(parents=True, exist_ok=True)
            features_train[qcirc.name] = create_feature_dict(qcirc)
            with feature_train_filepath.open("wb") as f:
                pickle.dump(features_train[qcirc.name], f)

    # Load and assign the labels
    print("-----------------------------------------")
    print("Loading labels...")
    with open(label_path, "rb") as f:
        label_data = pickle.load(f)
    print("-----------------------------------------")

    labels_train = {}
    for qc in train_circuits:
        if qc.name in label_data:
            labels_train[qc.name] = label_data[qc.name]
        else:
            # print(f"Skip this circuit: label for {qc.name} not found.")
            features_train.pop(qc.name, None)
    # for i in labels:
    #     print(f"Label: {i} \t {labels[i]}")
    print("Num. remaining features: ", len(features_train))
    print("Num. reamining labels: ", len(labels_train))

    print("-----------------------------------------")
    print("Formulating the X and x matrices for training...")
    X = np.array([list(circ_feat_dict.values()) for circ_feat_dict in features_train.values()], dtype=np.float32)
    y = np.array(list(labels_train.values()), dtype=np.float32)
    print("X shape: ", X.shape)
    print("y shape: ", y.shape)

    # Model setup
    # model = RandomForestRegressor(random_state=42)
    # model = RandomForestClassifier(random_state=42)
    model = MLPClassifier(solver='lbfgs', alpha=1e-5, hidden_layer_sizes=(5, 2), random_state=1)

    # Tuning hyperparameters for the model
    # NOTE: Can (and should) be adjusted for comprehensive search
    # grid = {
    #     "n_estimators": [10, 50],
    #     "max_depth": [5, 10],
        # "min_samples_split": [2, 3, 5, 10],
        # "min_samples_leaf": [1, 2, 3],
        # "min_weight_fraction_leaf": [0.0, 0.1],
        # "max_features": ["sqrt", "log2", None],
        # "max_leaf_nodes": [None, 10],
        # "min_impurity_decrease": [0.0, 0.1],
        # "bootstrap": [True],
        # "oob_score": [True],
        # "warm_start": [False, True],
        # "ccp_alpha": [0.0, 0.01, 0.1, 0.2],
        # "max_samples": [None, 0.5, 1.0],  
        # "criterion": ["absolute_error"],
        # "criterion": ['gini', 'entropy']
    # }
    mlp_grid = {
        "hidden_layer_sizes": [(5,), (10,), (5, 10)],
        "activation": ["relu", "tanh"],
        "solver": ["adam", "lbfgs"],
        "alpha": [0.0001, 0.001, 0.01],
        "learning_rate_init": [0.001, 0.01],
        "max_iter": [500]
    }
    kwargs = {
        "n_jobs": -1,
        "verbose": 1,
        "cv": StratifiedKFold(n_splits=5, shuffle=True, random_state=42)
    }
    grid_search = GridSearchCV(model, param_grid=mlp_grid, error_score="raise", **kwargs)

    print("-----------------------------------------")
    print("Training and tuing the model ...")
    grid_search.fit(X, y)

    print("-----------------------------------------")
    best_model = grid_search.best_estimator_
    # Quickly predict on same data
    y_pred  = best_model.predict(X)
    y_proba = best_model.predict_proba(X)[:, 1]

    print(" + Best parameters found:", grid_search.best_params_)
    print(" + Classification Report:")
    print(classification_report(y, y_pred))
    print(" + ROC AUC Score:", roc_auc_score(y, y_proba))
    print("-----------------------------------------")
    print("")

    print("Evaluate the best model on the test set ...")
    print(" + Loading the test set")
    test_circuits = []
    for name, circ in test_set:
        test_circuits.append(circ.decompose())
        # set circuit name for identification as an additional attribute
        test_circuits[-1].name = name
    print(" + Extracting features for the test set")
    features_test_dir = Path(f"../data/features/test")
    feature_test = {}
    for qcirc in test_circuits:
        feature_test_filepath = features_test_dir / f"{qcirc.name}.pkl"
        # Load feature dictionary if it exists
        if feature_test_filepath.exists():
            with feature_test_filepath.open("rb") as f:
                feature_test[qcirc.name] = pickle.load(f)
        # Otherwise, create and save feature dictionary
        else:
            feature_test_filepath.parent.mkdir(parents=True, exist_ok=True)
            feature_test[qcirc.name] = create_feature_dict(qcirc)
            with feature_test_filepath.open("wb") as f:
                pickle.dump(feature_test[qcirc.name], f)
    labels_test = {}
    for qc in test_circuits:
        if qc.name in label_data:
            labels_test[qc.name] = label_data[qc.name]
        else:
            # print(f"Skip this circuit: label for test {qc.name} not found.")
            feature_test.pop(qc.name, None)
    print("-----------------------------------------")
    X_test = np.array([list(circ_feat_dict.values()) for circ_feat_dict in feature_test.values()], dtype=np.float32)
    y_test = np.array(list(labels_test.values()), dtype=np.float32)
    # print("X_test shape: ", X_test.shape)
    # print("y_test shape: ", y_test.shape)

    y_test_pred  = best_model.predict(X_test)
    y_test_proba = best_model.predict_proba(X_test)[:, 1]
    print(" + Test Classification Report:")
    print(classification_report(y_test, y_test_pred))
    print(" + Test ROC AUC Score:", roc_auc_score(y_test, y_test_proba))
    print("-----------------------------------------")
    print("")

    # print("-----------------------------------------")
    # print("Saving the best model to file ...")
    # model_filepath = f"../data/trained_models/{model_name}.onnx"
    # initial_type = [("float_input", FloatTensorType([None, X.shape[1]]))]
    # onnx_model = convert_sklearn(grid_search.best_estimator_, initial_types=initial_type)
    # with open(model_filepath, "wb") as f:
    #     f.write(onnx_model.SerializeToString())
    # print(f"{model_filepath}")
    # onnx_formated_model = grid_search.best_estimator_
    # print("-----------------------------------------")

    return 0

# -------------------------------------------------
# Main part
# -------------------------------------------------
qubits = [1,2]
lengths = [1]
num_samples = 2
backends = ['Q20', 'QExa20']
list_circuits = '../data/qasm_circuits/benchmark_list_quasm.pkl'
list_labels   = '../data/labels/labels.pkl'

# Load the benchmark datasets
with open(list_circuits, 'rb') as f:
    circuits_list = pickle.load(f)

print("-----------------------------------------")
print("Number of circuits: ", len(circuits_list))
print("3 example circuits: ")
for i in range(3):
    print(circuits_list[i][1])
print("-----------------------------------------")

paths = []
for backend in backends:
    path = 'random_' + backend + '_counts.pkl'
    paths.append(path)
print("Paths to the counts data: ", paths)
print("-----------------------------------------")

# Train the model
# model_name = 'rf_regressor'
# model_name = 'rf_classifier'
model_name = 'mlp_classifier'
trained_model = train_randomforest_model(model_name, list_circuits, list_labels)