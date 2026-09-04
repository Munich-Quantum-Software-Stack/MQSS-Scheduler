from __future__ import annotations

import logging
import sys

from typing import TYPE_CHECKING, Any
from pathlib import Path
from collections.abc import Callable

import warnings
import numpy as np

from typing import cast
from joblib import load
from gymnasium import Env
from gymnasium.spaces import Box, Dict, Discrete

from qiskit import QuantumCircuit
from qiskit.passmanager.flow_controllers import DoWhileController
from qiskit.transpiler import CouplingMap, PassManager, Target, TranspileLayout
from qiskit.transpiler.passes import CheckMap, GatesInBasis
from qiskit.transpiler.passes.layout.vf2_layout import VF2LayoutStopReason

from reward import Reward, create_feature_dict, calc_supermarq_features, get_state_sample
from action import get_actions_by_pass_type, Action, PassType, CompilationOrigin, DeviceIndependentAction, DeviceDependentAction

mqss_predictor_env_logger = logging.getLogger("mqss-predictor-env")
mqss_predictor_env_logger.setLevel(logging.INFO)

class  MQSSPredictorEnv(Env):

    def __init__(
            self,
            device: Target,
            reward_function: Reward,
            path_training_circuits: Path | None = None
    ):
        mqss_predictor_env_logger.info("Initializing MQSS Predictor Environment")
        self.path_training_circuits = path_training_circuits
        self.action_set = {}
        self.actions_synthesis_indices = []
        self.actions_layout_indices = []
        self.actions_routing_indices = []
        self.actions_mapping_indices = []
        self.actions_opt_indices = []
        self.actions_final_optimization_indices = []
        self.action_terminate_index = []
        self.used_actions: list[str] = []
        self.device = device

        index = 0
        action_dict = get_actions_by_pass_type()

        for pass_type, passes in action_dict.items():
            print(f"{pass_type}")
            for pass_action in passes:
                action_name = pass_action.name
                print(f"  + Pass: {action_name}")

        for elem in action_dict[PassType.SYNTHESIS]:
            self.action_set[index] = elem
            self.actions_synthesis_indices.append(index)
            index += 1
        for elem in action_dict[PassType.LAYOUT]:
            self.action_set[index] = elem
            self.actions_layout_indices.append(index)
            index += 1
        for elem in action_dict[PassType.ROUTING]:
            self.action_set[index] = elem
            self.actions_routing_indices.append(index)
            index += 1
        for elem in action_dict[PassType.OPT]:
            self.action_set[index] = elem
            self.actions_opt_indices.append(index)
            index += 1
        for elem in action_dict[PassType.MAPPING]:
            self.action_set[index] = elem
            self.actions_mapping_indices.append(index)
            index += 1
        for elem in action_dict[PassType.FINAL_OPT]:
            self.action_set[index] = elem
            self.actions_final_optimization_indices.append(index)
            index += 1

        # Add the terminate pass action
        self.action_set[index] = action_dict[PassType.TERMINATE][0]
        self.action_terminate_index.append(index)

        # print(f"Total number of actions: {len(self.action_set)} | {self.action_set}")
        print(f"Actions synthesis: {self.actions_synthesis_indices}")
        print(f"Actions layout: {self.actions_layout_indices}")
        print(f"Actions routing: {self.actions_routing_indices}")
        print(f"Actions mapping: {self.actions_mapping_indices}")
        print(f"Actions optimization: {self.actions_opt_indices}")
        print(f"Actions final optimization: {self.actions_final_optimization_indices}")
        print(f"Action terminate: {self.action_terminate_index}")
        print(f"--------------------------------")
            
        # add reward function
        self.reward_function = reward_function
        self.action_space = Discrete(len(self.action_set.keys()))
        self.num_steps = 0
        self.layout: TranspileLayout | None = None
        self.num_qubits_uncompiled_circuit = 0
        self.has_parameterized_gates = False
        self.rng = np.random.default_rng(10)
        print(f"Reward function: {self.reward_function}")
        print(f"Action space: {self.action_space}")
        print(f"--------------------------------")

        spaces = {
            "num_qubits": Discrete(128),
            "depth": Discrete(1000000),
            "program_communication": Box(low=0, high=1, shape=(1,), dtype=np.float32),
            "critical_depth": Box(low=0, high=1, shape=(1,), dtype=np.float32),
            "entanglement_ratio": Box(low=0, high=1, shape=(1,), dtype=np.float32),
            "parallelism": Box(low=0, high=1, shape=(1,), dtype=np.float32),
            "liveness": Box(low=0, high=1, shape=(1,), dtype=np.float32),
        }
        self.observation_space = Dict(spaces)
        self.filename = ""

    def reset(self, 
              qc: Path | str | QuantumCircuit | None = None,
              seed: int | None = None,
              options: dict[str, Any] | None = None) -> tuple[QuantumCircuit, dict[str, Any]]:
        
        super().reset(seed=seed)

        if isinstance(qc, QuantumCircuit):
            self.state = qc
        elif qc:
            self.state = QuantumCircuit.from_qasm_file(str(qc))
        else:
            self.state, self.filename = get_state_sample(self.device.num_qubits, self.path_training_circuits, self.rng)

        self.action_space = Discrete(len(self.action_set.keys()))
        self.num_steps = 0
        self.used_actions = []
        self.layout = None
        self.valid_actions = self.actions_opt_indices + self.actions_synthesis_indices
        self.error_occurred = False
        self.num_qubits_uncompiled_circuit = self.state.num_qubits
        self.has_parameterized_gates = len(self.state.parameters) > 0

        return create_feature_dict(self.state), {}

    
    def step(self, action: int) -> tuple[dict[str, Any], float, bool, bool, dict[Any, Any]]:
        """Executes the given action and returns the new state, the reward, whether the episode is done, whether the episode 
        is truncated and additional information.

        Arguments:
            action: The action to be executed, represented by its index in the action set.

        Returns:
            A tuple containing the new state as a feature dictionary, the reward value, whether the episode is done, whether 
            the episode is truncated, and additional information.

        Raises:
            RuntimeError: If no valid actions are left.
        """
        if action not in self.action_set:
            raise ValueError(f"Invalid action: {action}.\
                             Available actions: {self.action_set.keys()}")

        action_idx = action
        self.used_actions.append(str(self.action_set[action_idx].name))
        altered_qc = self.apply_action(action_idx)
        
        if not altered_qc:
            return (
                create_feature_dict(self.state),
                0,
                True,
                False,
                {},
            )

        self.state: QuantumCircuit = altered_qc
        self.num_steps += 1

        self.valid_actions = self.determine_valid_actions_for_state()
        if len(self.valid_actions) == 0:
            msg = "No valid actions left."
            raise RuntimeError(msg)

        if action_idx == self.action_terminate_index:
            reward_val = self.calculate_reward()
            done = True
        else:
            reward_val = 0
            done = False

        # in case the Qiskit.QuantumCircuit has unitary or u gates in it, decompose them 
        # (because otherwise qiskit will throw an error when applying the BasisTranslator
        if self.state.count_ops().get("unitary"):
            self.state = self.state.decompose(gates_to_decompose="unitary")

        self.state._layout = self.layout  # noqa: SLF001
        obs = create_feature_dict(self.state)

        return obs, reward_val, done, False, {}

    def apply_action(self, action_idx: int) -> QuantumCircuit | None:
        """Applies the action corresponding to the given index to the current quantum circuit state.

        Arguments:
            action_idx: The index of the action to be applied.

        Returns:
            The altered quantum circuit after applying the action, or None if no valid action could be applied.
        """
        if action_idx not in self.action_set:
            raise ValueError(f"Invalid action index: {action_idx}. \
                             Available actions: {self.action_set.keys()}")
        
        action = self.action_set[action_idx]

        if action.name == "terminate":
            return self.state
        elif action.origin == CompilationOrigin.QISKIT:
            return self.apply_qiskit_action(action, action_idx)
        else:
            raise ValueError(f"General pass type: {action.type}")
        
    def apply_qiskit_action(self, action: Action, action_idx: int) -> QuantumCircuit | None:
        if action.name == "QiskitO3" and isinstance(action, DeviceDependentAction):
            passes = action.transpile_pass(
                self.device.operation_names,
                CouplingMap(self.device.build_coupling_map()) if self.layout else None,
            )
            pm = PassManager([DoWhileController(passes, do_while=action.do_while)])
        else:
            transpile_pass = (
                action.transpile_pass(self.device) if callable(action.transpile_pass) else action.transpile_pass
            )
            pm = PassManager(transpile_pass)

        altered_qc = pm.run(self.state)

        if action_idx in (
            self.actions_layout_indices + self.actions_mapping_indices + self.actions_final_optimization_indices
        ):
            altered_qc = self._handle_qiskit_layout_postprocessing(action, pm, altered_qc)

        elif action_idx in self.actions_routing_indices and self.layout:
            self.layout.final_layout = pm.property_set["final_layout"]

        return altered_qc