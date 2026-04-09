from __future__ import annotations

import logging
import zipfile
import numpy as np
import networkx as nx


from typing import TYPE_CHECKING, Literal
from pathlib import Path
from joblib import load
from numpy.random import Generator
from numpy.typing import NDArray

from qiskit import QuantumCircuit, QuantumRegister
from qiskit.circuit import Qubit
from qiskit.transpiler import Target

# lazy import of qiskit transpiler
from qiskit.transpiler import InstructionDurations, Layout, PassManager, passes  # noqa: PLC0415
from qiskit.transpiler.passes import ApplyLayout, SetLayout  # noqa: PLC0415
from qiskit.converters import circuit_to_dag

Reward = Literal[
    "expected_fidelity",
    "critical_depth",
    "estimated_success_probability",
    "hellinger_distance",
    "estimated_hellinger_distance",
]

class SupermarqFeatures:
    """Data class for the Supermarq features of a quantum circuit."""

    program_communication: float
    critical_depth: float
    entanglement_ratio: float
    parallelism: float
    liveness: float


def calc_supermarq_features(
    qc: QuantumCircuit,
) -> SupermarqFeatures:
    """Calculates the Supermarq features for a given quantum circuit. 
    Code adapted from https://github.com/Infleqtion/client-superstaq/blob/91d947f8cc1d99f90dca58df5248d9016e4a5345/supermarq-benchmarks/supermarq/converters.py."""
    
    num_qubits = qc.num_qubits
    dag = circuit_to_dag(qc)
    dag.remove_all_ops_named("barrier")

    # Program communication = circuit's average qubit degree / degree of a complete graph.
    graph = nx.Graph()
    for op in dag.two_qubit_ops():
        q1, q2 = op.qargs
        graph.add_edge(qc.find_bit(q1).index, qc.find_bit(q2).index)
    degree_sum = sum(graph.degree(n) for n in graph.nodes)
    program_communication = degree_sum / (num_qubits * (num_qubits - 1)) if num_qubits > 1 else 0

    # Liveness feature = sum of all entries in the liveness matrix / (num_qubits * depth).
    activity_matrix = np.zeros((num_qubits, dag.depth()))
    for i, layer in enumerate(dag.layers()):
        for op in layer["partition"]:
            for qubit in op:
                activity_matrix[qc.find_bit(qubit).index, i] = 1
    liveness = np.sum(activity_matrix) / (num_qubits * dag.depth()) if dag.depth() > 0 else 0

    #  Parallelism feature = max((((# of gates / depth) -1) /(# of qubits -1)), 0).
    parallelism = (
        max(((len(dag.gate_nodes()) / dag.depth()) - 1) / (num_qubits - 1), 0)
        if num_qubits > 1 and dag.depth() > 0
        else 0
    )

    # Entanglement-ratio = ratio between # of 2-qubit gates and total number of gates in the circuit.
    entanglement_ratio = len(dag.two_qubit_ops()) / len(dag.gate_nodes()) if len(dag.gate_nodes()) > 0 else 0

    # Critical depth = # of 2-qubit gates along the critical path / total # of 2-qubit gates.
    longest_paths = dag.count_ops_longest_path()
    n_ed = sum(longest_paths[name] for name in {op.name for op in dag.two_qubit_ops()} if name in longest_paths)
    n_e = len(dag.two_qubit_ops())
    critical_depth = n_ed / n_e if n_e != 0 else 0

    assert 0 <= program_communication <= 1
    assert 0 <= critical_depth <= 1
    assert 0 <= entanglement_ratio <= 1
    assert 0 <= parallelism <= 1
    assert 0 <= liveness <= 1

    return SupermarqFeatures(
        program_communication,
        critical_depth,
        entanglement_ratio,
        parallelism,
        liveness,
    )


def crit_depth(qc: QuantumCircuit, precision: int = 10) -> float:
    """Calculates the critical depth of a given quantum circuit."""
    supermarq_features = calc_supermarq_features(qc)
    return float(np.round(1 - supermarq_features.critical_depth, precision).item())


def expected_fidelity(qc: QuantumCircuit, device: Target, precision: int = 10) -> float:
    """Calculates the expected fidelity of a given quantum circuit on a given device.

    Arguments:
        qc: The quantum circuit to be compiled.
        device: The device to be used for compilation.
        precision: The precision of the returned value. Defaults to 10.

    Returns:
        The expected fidelity of the given quantum circuit on the given device.
    """
    res = 1.0
    for qc_instruction in qc.data:
        instruction, qargs = qc_instruction.operation, qc_instruction.qubits
        gate_type = instruction.name

        if gate_type != "barrier":
            assert len(qargs) in [1, 2]
            first_qubit_idx = calc_qubit_index(qargs, qc.qregs, 0)

            if len(qargs) == 1:
                specific_fidelity = 1 - device[gate_type][first_qubit_idx,].error
            else:
                second_qubit_idx = calc_qubit_index(qargs, qc.qregs, 1)
                specific_fidelity = 1 - device[gate_type][first_qubit_idx, second_qubit_idx].error

            res *= specific_fidelity

    return float(np.round(res, precision).item())


def calc_qubit_index(qargs: list[Qubit], qregs: list[QuantumRegister], index: int) -> int:
    """Calculates the global qubit index for a given quantum circuit and qubit index.

    Arguments:
        qargs: The qubits of the quantum circuit.
        qregs: The quantum registers of the quantum circuit.
        index: The index of the qubit in the qargs list.

    Returns:
        The global qubit index of the given qubit in the quantum circuit.

    Raises:
        ValueError: If the qubit index is not found in the quantum registers.
    """
    offset = 0
    for reg in qregs:
        if qargs[index] not in reg:
            offset += reg.size
        else:
            qubit_index: int = offset + reg.index(qargs[index])
            return qubit_index
    error_msg = f"Global qubit index for local qubit {index} index not found."
    raise ValueError(error_msg)


def estimated_success_probability(qc: QuantumCircuit, device: Target, precision: int = 10) -> float:
    """Calculates the estimated success probability of a given quantum circuit on a given device.

    It is calculated by multiplying the expected fidelity with a min(T1,T2)-dependent
    decay factor during qubit idle times. To this end, the circuit is scheduled using ASAP scheduling.

    Arguments:
        qc: The quantum circuit to be compiled.
        device: The device to be used for compilation.
        precision: The precision of the returned value. Defaults to 10.

    Returns:
        The expected success probability of the given quantum circuit on the given device.
    """

    # collect gate and measurement durations for active qubits
    op_times, active_qubits = [], set()
    for instr in qc.data:
        instruction = instr.operation
        qargs = instr.qubits
        gate_type = instruction.name

        if gate_type == "barrier" or gate_type == "id":
            continue
        assert len(qargs) in (1, 2)
        first_qubit_idx = calc_qubit_index(qargs, qc.qregs, 0)
        active_qubits.add(first_qubit_idx)

        if len(qargs) == 1:  # single-qubit gate
            duration = device[gate_type][first_qubit_idx,].duration
            op_times.append((
                gate_type,
                [
                    first_qubit_idx,
                ],
                duration,
                "s",
            ))
        else:  # multi-qubit gate
            second_qubit_idx = calc_qubit_index(qargs, qc.qregs, 1)
            active_qubits.add(second_qubit_idx)
            duration = device[gate_type][first_qubit_idx, second_qubit_idx].duration
            op_times.append((gate_type, [first_qubit_idx, second_qubit_idx], duration, "s"))

    # check whether the circuit was transformed by tket (i.e. changed register name)
    # qiskit ASAPScheduleAnalysis expects all qubit registers to be named 'q'
    if qc.qregs[0].name != "q":
        # create a layout that maps the (tket) 'node' registers to the (qiskit) 'q' registers
        layouts = [SetLayout(Layout({node_qubit: i for i, node_qubit in enumerate(node_reg)})) for node_reg in qc.qregs]
        # create a pass manager with the SetLayout and ApplyLayout passes
        pm = PassManager(list(layouts))
        pm.append(ApplyLayout())

        # replace the 'node' register with the 'q' register in the circuit
        qc = pm.run(qc)
        assert qc.qregs[0].name == "q"

    # associate gate and idle (delay) times for each qubit through asap scheduling
    sched_pass = passes.ASAPScheduleAnalysis(InstructionDurations(op_times))
    delay_pass = passes.PadDelay()
    pm = PassManager([sched_pass, delay_pass])
    scheduled_circ = pm.run(qc)

    res = 1.0
    for instr in scheduled_circ.data:
        instruction = instr.operation
        qargs = instr.qubits
        gate_type = instruction.name

        if gate_type == "barrier" or gate_type == "id":
            continue

        assert len(qargs) in (1, 2)
        first_qubit_idx = calc_qubit_index(qargs, qc.qregs, 0)

        if len(qargs) == 1:
            if gate_type == "measure":
                res *= 1 - device[gate_type][first_qubit_idx,].error
                continue

            if gate_type == "delay":
                # only consider active qubits
                if first_qubit_idx not in active_qubits:
                    continue

                res *= np.exp(
                    -instruction.duration
                    / min(device.qubit_properties[first_qubit_idx].t1, device.qubit_properties[first_qubit_idx].t2)
                )
                continue

            res *= 1 - device[gate_type][first_qubit_idx,].error
        else:
            second_qubit_idx = calc_qubit_index(qargs, qc.qregs, 1)
            res *= 1 - device[gate_type][first_qubit_idx, second_qubit_idx].error

    return float(np.round(res, precision).item())


def create_feature_dict(qc: QuantumCircuit) -> dict[str, int | NDArray[np.float64]]:
    """Creates a feature dictionary for a given quantum circuit.

    Arguments:
        qc: The quantum circuit for which the feature dictionary is created.

    Returns:
        The feature dictionary for the given quantum circuit.
    """
    feature_dict = {
        "num_qubits": qc.num_qubits,
        "depth": qc.depth(),
    }

    # calculate Supermarq features
    supermarq_features = calc_supermarq_features(qc)

    # for all dict values, put them in a list each
    feature_dict["program_communication"] = np.array([supermarq_features.program_communication], dtype=np.float32)
    feature_dict["critical_depth"] = np.array([supermarq_features.critical_depth], dtype=np.float32)
    feature_dict["entanglement_ratio"] = np.array([supermarq_features.entanglement_ratio], dtype=np.float32)
    feature_dict["parallelism"] = np.array([supermarq_features.parallelism], dtype=np.float32)
    feature_dict["liveness"] = np.array([supermarq_features.liveness], dtype=np.float32)

    return feature_dict

def get_state_sample(max_qubits: int, path_training_circuits: Path, rng: Generator) -> tuple[QuantumCircuit, str]:
   
    file_list = list(path_training_circuits.glob("*.qasm"))

    path_zip = path_training_circuits / "training_data_compilation.zip"

    if len(file_list) == 0 and path_zip.exists():
        with zipfile.ZipFile(str(path_zip), "r") as zip_ref:
            zip_ref.extractall(path_training_circuits)

        file_list = list(path_training_circuits.glob("*.qasm"))
        assert len(file_list) > 0

    found_suitable_qc = False
    while not found_suitable_qc:
        random_index = rng.integers(len(file_list))
        num_qubits = int(str(file_list[random_index]).split("_")[-1].split(".")[0])
        if max_qubits and num_qubits > max_qubits:
            continue
        found_suitable_qc = True

    try:
        qc = QuantumCircuit.from_qasm_file(str(file_list[random_index]))
    except Exception:
        raise RuntimeError("Could not read QuantumCircuit from: " + str(file_list[random_index])) from None

    return qc, str(file_list[random_index])
