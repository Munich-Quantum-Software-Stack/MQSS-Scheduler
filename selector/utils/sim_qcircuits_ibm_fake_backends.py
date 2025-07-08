import os
import ast
import json
import time
import pickle
import argparse

import numpy as np
import pandas as pd

from typing import List, Tuple
from pathlib import Path

from qiskit import qpy
from qiskit import QuantumCircuit, transpile
from qiskit.converters import circuit_to_dag
from qiskit_ibm_runtime import SamplerV2
from qiskit_ibm_runtime.fake_provider import (
    FakeAlmadenV2,     
    FakeAthensV2,
    FakeAuckland,     
    FakeBelemV2,
    FakeBrooklynV2,
    FakeCairoV2,
    FakeCasablancaV2,
    FakeCambridgeV2,   
    FakeGuadalupeV2,   
    FakeMelbourneV2,
    FakePrague,      
    FakeYorktownV2
)
from qiskit_aer import AerSimulator

# ------------------------------------------------
# Util functions
# ------------------------------------------------

def execute_circuit(qcircuit: QuantumCircuit, backends, nshots, dir_path):
    """
    For each circuit, execute it on the specified backend and return the results.
    The results of each circuit are stored in a json file with dictionary format:
        filename: circname_nqubits_nshots.json
        {
            "circuit_name": name,
            "num_qubits": nqubits,
            "shots": nshots,
            "counts_backend1": {"0": 500,"1": 500},
            "counts_backend2": {"0": 600,"1": 400},
            ...
        }

    For further execution or experiments, with new backends to run, we can just add a new field to the dictionary
    with the new backend name and the results.
    """

    num_available_backends = len(backends)
    qcirc_execdata_dict = {
        "circuit_name": qcircuit.name,
        "num_qubits": qcircuit.num_qubits,
        "num_shots": nshots
    }

    for i in range(num_available_backends):
        backend = backends[i]
        print(f'Executing circuit {qcircuit.name} on backend: {backend.name}')
        
        # Execute the circuit on the backend
        sampler = SamplerV2(backend)
        try:
            transpiled_circuit = transpile(qcircuit, backend)
            job = sampler.run([transpiled_circuit], shots=nshots)
            result = job.result()[0]
            counts = result.data.meas.get_counts()
            print(f'Results: {counts}')

            # add results field to the dict data
            field_name = "counts_" + backend.name
            qcirc_execdata_dict[field_name] = counts

        except Exception as e:
            print(f'Error executing on backend {backend.name}: {e}')
            result = None
            counts = None
            # print(f'Results: {counts}')

            # add results field to the dict data
            field_name = "counts_" + backend.name
            qcirc_execdata_dict[field_name] = counts
            continue
        
    # Save the results in a json file
    print(qcirc_execdata_dict)
    output_file = f'{qcircuit.name}_{qcircuit.num_qubits}_{nshots}.json'
    with open(dir_path / output_file, 'w') as f:
        json.dump(qcirc_execdata_dict, f, indent=4)
    print(f'Results saved to {dir_path / output_file}')
           
    return 0

# ------------------------------------------------
# Main function to run circuits on fake backends
# ------------------------------------------------
if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Run quantum circuits on fake backends.")
    parser.add_argument("--input", "-i", type=str, required=True, help="Path to the input circuits")
    # parser.add_argument("--output", "-o", type=str, default="Tables/Simulated_Fake_RB.csv", help="Output CSV file path")
    args = parser.parse_args()
    
    set_backends = [
        AerSimulator(),
        FakeAlmadenV2(),
        FakeAthensV2(),
        FakeAuckland(),
        FakeBelemV2(),
        FakeBrooklynV2(),
        FakeCairoV2(),
        FakeCasablancaV2(),
        FakeCambridgeV2(),
        FakeGuadalupeV2(),
        FakeMelbourneV2(),
        FakePrague(),
        FakeYorktownV2()
    ]

    try:
        input_path = Path(args.input)
        print(f'----------------------------------')
        print(f'Run circuits in folder: {input_path}')
        print(f'----------------------------------')
        input_files = os.listdir(input_path)
        for f in input_files:
            file_path = input_path / f
            print(f'Loading circuit from: {file_path}')
            with open(input_path / f, 'rb') as fd:
                input_circuit = qpy.load(fd)[0]
                input_circuit.name = (f.split('.')[0]).split('_')[0]
                
                # in case of dj and qpeexact circuits, measure all qubits 
                if input_circuit.name == 'dj' or input_circuit.name == 'qpeexact':
                    input_circuit.measure_all()
                # print(input_circuit)

                # execute the circuit
                execute_circuit(input_circuit, set_backends, 1000, input_path)

        # df_wide = benchmark_on_fakes_wide(con_circuits, fake_backends=set_fakes, shots=1000000, path=str(output_path))
        
    except Exception as e:
        print(f'Error: {e}')

    
