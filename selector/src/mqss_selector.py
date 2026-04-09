
import argparse
from mqss.scheduler.selector.src.mqss_dev_pass_predictor import *
from target_predictor import *
from qiskit import QuantumCircuit


def parse_args():
    parser = argparse.ArgumentParser(
        description="MQSS Selector: Predicts optimal quantum device and transpiler passes."
    )

    parser.add_argument(
        "-c", "--circuit", type=str, required=True,
        help="Path to the quantum circuit file (e.g., .qasm or Python script)."
    )
    parser.add_argument(
        "--simulate", action="store_true",
        help="Simulate the optimized circuit after prediction."
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Print detailed output and logs."
    )
    return parser.parse_args()


def load_circuit(circuit_path):
    if circuit_path.endswith(".qasm"):
        qc = QuantumCircuit.from_qasm_file(circuit_path)
    elif circuit_path.endswith(".py"):
        # Dynamically load user-defined circuit from Python script
        globals_dict = {}
        with open(circuit_path) as f:
            exec(f.read(), globals_dict)
        qc = globals_dict.get("qc", None)
        if qc is None or not isinstance(qc, QuantumCircuit):
            raise ValueError("No valid QuantumCircuit named 'qc' found in script.")
    else:
        raise ValueError("Unsupported file type. Use .qasm or .py file containing a 'qc' QuantumCircuit.")

    return qc


def main():
    args = parse_args()

    if args.verbose:
        print(f"Loading circuit from: {args.circuit}")

    qc = load_circuit(args.circuit)

    if args.verbose:
        print(f"Circuit loaded with {qc.num_qubits} qubits and {qc.count_ops()} operations.")

    # Predict the best device/target
    best_target = predict_target(qc)
    if args.verbose:
        print(f"Best target predicted: {best_target}")

    # Predict the optimal transpiler passes
    best_passes = predict_passes(qc, target=best_target)
    if args.verbose:
        print(f"Passes selected: {best_passes}")

    # Optionally simulate
    if args.simulate:
        print("Simulating optimized circuit... (simulation not implemented here)")

    print("MQSS Selection complete!")


if __name__ == "__main__":
    main()
