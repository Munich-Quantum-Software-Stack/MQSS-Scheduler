# ------------------------------------------------------------------------------
# Copyright 2024 Munich Quantum Software Stack Project
#
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
# "License"); you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/develop/LICENSE
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------ 

import os
import re
import sys
import ast
import json
import pickle
import random
import datetime
import argparse

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from typing import List, Tuple, Dict
from pathlib import Path
from collections import defaultdict

from qiskit_aer import Aer

from qiskit import qasm3, QuantumCircuit
from qiskit.converters import circuit_to_dag
from qiskit.visualization import plot_histogram, plot_coupling_map

from qiskit_experiments.library import StandardRB
from qiskit_experiments.framework import ExperimentData

from mqp.qiskit_provider import MQPProvider
from mqp.qiskit_provider.job import MQPJob


def generate_rb_circuits(qubits:List[int], lengths:List[int], num_samples:int,
                         path:str, seed=42) -> List[Tuple[str, QuantumCircuit]]:
    """!
    @brief Generates RB circuits based on randomized rotation gates and saves them to a file.

    This function creates a set of random benchmarking circuits based on the provided 
    parameters and saves them as a  list of tuples containing the circuit name 
    and the corresponding QuantumCircuit object.

    @param qubits A list of integers representing the qubits for the benchmarking circuits.
    @param lengths A list of integers representing the lengths of the benchmarking sequences.
    @param num_samples The number of random benchmarking circuits to generate for each qubit size and length.
    @param path The file path where the  circuits will be saved.
    @param seed The seed for random number generation (default is 42).
    
    @return A list of tuples, where each tuple contains:
        - A string representing the name of the circuit (e.g., "circuit_1").
        - A QuantumCircuit object representing the generated benchmarking circuit.
    """

    rb_circuits = []
    for q in qubits:
        list_qubits = qubits[:q]
        rb = StandardRB(list_qubits, lengths, num_samples=num_samples, seed=seed)
        rb_circuits.append(rb.circuits())

    rb_circuits = [cir for sublist in rb_circuits for cir in sublist]
    list_circuits = [(f"circuit_{i+1}", qasm3.dumps(circuit.decompose())) \
                     for i, circuit in enumerate(rb_circuits)]
    with open(path, 'w') as f:
        json.dump(list_circuits, f, indent=2)

    print(f"Generated {len(list_circuits)} circuits and saved to {path}")

    return list_circuits



# --------------------------------------------------------------------
# Test function to generate random benchmarking circuits
# ---------------------------------------------------------------------
if __name__ == '__main__':

    qubits = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    lengths = [1, 2]
    num_samples = 2
    path = './rb_benchmark_circuits.json'
    # The number of rb circuits:
    #   num_qubits * num_lengths * num_samples
    #   = 2 qubits * 2 lengths * 2 samples = 8 circuits

    generate_rb_circuits(qubits, lengths, num_samples, path)

