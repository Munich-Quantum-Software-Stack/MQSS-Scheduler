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

from qiskit import qpy
from qiskit import qasm2, qasm3, QuantumCircuit
from qiskit.converters import circuit_to_dag
from qiskit.visualization import plot_histogram, plot_coupling_map

from qiskit_experiments.library import StandardRB
from qiskit_experiments.framework import ExperimentData

from mqt.bench.benchmark_generator import get_benchmark, get_supported_benchmarks

def generate_mqtbench_circuits(qubits:List[int], algorithms:List[str], path:str, seed=42) \
                        -> List[Tuple[str, QuantumCircuit]]:
    """!
    @brief Generates MQT Bench circuits.

    This function creates a set of supported benchmarking algorithms circuits from the MQT Bench library.

    @param qubits A list of integers representing the qubits for the benchmarking circuits.
    @param circuit_size A list of integers representing the lengths of the benchmarking sequences.
    @param path The file path where the  circuits will be saved.
    @param seed The seed for random number generation (default is 42).
    
    @return A list of tuples, where each tuple contains:
        - A string representing the name of the circuit (e.g., "circuit_1").
        - A QuantumCircuit object representing the generated benchmarking circuit.
    """

    list_circuits = []
    for nqubits in qubits:
        if nqubits <= 2:
            selc_algorithms = [alg for alg in algorithms \
                    if alg != "graphstate" and alg != "qaoa" and alg != "vqe"]
            print(f"Selected algorithms for nqubits <= 2: {selc_algorithms}")
            print(f"-------------------------------------------------------")
        else:
            selc_algorithms = algorithms
            print(f"Selected algorithms for nqubits = {nqubits}: {selc_algorithms}")
            print(f"-------------------------------------------------------")
        # Generate circuits for each algorithm
        for algorithm in selc_algorithms:
            if algorithm not in get_supported_benchmarks():
                raise ValueError(f"Algorithm '{algorithm}' is not supported. "
                                f"Supported algorithms are: {get_supported_benchmarks()}")
    
            benchmark_circ = get_benchmark(benchmark_name=algorithm, level="alg", circuit_size=nqubits)
            circuit_name = f"{algorithm}_nqubits_{nqubits}"
            directory = os.path.join(path, f"mqtbench_circuits/{nqubits}_qubits/")
            if not os.path.exists(directory):
                os.makedirs(directory)
            # qasm2_file = open(directory + f"{circuit_name}"+".qasm2", "wb")
            # qasm3_file = open(directory + f"{circuit_name}"+".qasm3", "wb")
            qpy_file = open(directory + f"{circuit_name}"+".qpy", "wb")
            # qasm2.dump(benchmark_circ, qasm2_file)
            # qasm3.dump(benchmark_circ, qasm3_file)
            qpy.dump(benchmark_circ, qpy_file)
            # qasm2_file.close()
            # qasm3_file.close()
            qpy_file.close()

            list_circuits.append([circuit_name, nqubits, qpy_file.name])
    
    # Generate the pandas dataframe
    df_circuits = pd.DataFrame(list_circuits, columns=['circuit_name', 'nqubits', 'circuit_file'])
    df_mqtbench_circuits_file = 'mqtbench_circuits.csv'
    df_circuits.to_csv(path+df_mqtbench_circuits_file, index=False) 

    return df_circuits


# --------------------------------------------------------------------
# Test function to generate random benchmarking circuits
# ---------------------------------------------------------------------
if __name__ == '__main__':
    
    # Get list of supported algorithms for generate the benchmark circuits
    print(f"-------------------------------------------------------")
    supported_algorithms = get_supported_benchmarks()
    print(f" MQT Bench Supported Algorithms:")
    print(f"{supported_algorithms}")
    print(f"-------------------------------------------------------")

    # Define the qubits and algorithms for the circuits
    qubits = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
    path = '../data/benchmark_circuits/'
    selected_algorithms = ['ae', 'dj', 'grover-noancilla', 'ghz', 'graphstate', 
                        'qaoa', 'qft', 'qnn', 'qpeexact',
                        'random', 'vqe', 'wstate']
    # for graphstate, mqtbench will call nx.random_regular_graph(degree, num_qubits)
    # the default degree is 2, therefore, the num_qubits should be > 2
    df_mqtbench_circuits = generate_mqtbench_circuits(qubits, selected_algorithms, path)
    print(df_mqtbench_circuits)
    print(f"-------------------------------------------------------")