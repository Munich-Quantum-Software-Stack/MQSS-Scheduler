from __future__ import annotations

import re
import pytest

from typing import TYPE_CHECKING, Any, Literal
from pathlib import Path
from mqt.bench import BenchmarkLevel, get_benchmark
from mqt.bench.targets import get_device

from qiskit.circuit.library import CXGate
from qiskit.qasm2 import dump
from qiskit.transpiler import InstructionProperties, Target

from reward import Reward, get_state_sample
from mqss_dev_pass_predictor import MQSSDevPassPredictor

def test_qcompile_with_newly_trained_models() -> None:
    """Test the qcompile function with a newly trained model.

    Important: Those trained models are used in later tests and must not be deleted.
    To test ESP as well, training must be done with a device that provides all relevant information (i.e. T1, T2 and gate times).
    """
    reward = "expected_fidelity"
    device = get_device("ibm_falcon_127")
    qc = get_benchmark("ghz", BenchmarkLevel.ALG, 3)
    print(f"Benchmark circuit: {qc}")

    predictor = MQSSDevPassPredictor(reward=reward, device=device)

    model_name = "model_" + reward + "_" + device.description
    model_path = Path("./" + (model_name + ".zip"))
    print(f"Model name: {model_name}")
    print(f"Model path: {model_path}")

    predictor.train_model(
        timesteps=100,
        test=True,
    )

    # qc_compiled, compilation_information = rl_compile(qc, device=device, figure_of_merit=figure_of_merit)

    # assert qc_compiled.layout is not None
    # assert compilation_information is not None

def test_predictor_steps() -> None:
    """Test the predictor steps."""
    reward = "expected_fidelity"
    device = get_device("ibm_falcon_127")
    qc = get_benchmark("ghz", BenchmarkLevel.ALG, 3)
    predictor = MQSSDevPassPredictor(reward=reward, device=device)

    # test the environment steps
    obs, info = predictor.env.step(action=2)
    print(f"Observation: {obs}")
    print(f"Info: {info}")


def test_predictor_reset() -> None:
    """Test the predictor reset."""
    reward = "expected_fidelity"
    device = get_device("ibm_falcon_127")
    predictor = MQSSDevPassPredictor(reward=reward, device=device)

    # test the environment reset
    obs, info = predictor.env.reset()
    print(f"Observation: {obs}")
    print(f"Info: {info}")

if __name__ == "__main__":
    
    # test_qcompile_with_newly_trained_models()

    # test_predictor_steps()

    test_predictor_reset()