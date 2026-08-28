# MQSS-Selector

## MQSS Device & Pass Selector for Quantum Circuit Optimization & Compilation

Quantum computing circuits often require specific backend configurations and optimization passes tailored to the circuit's structure and requirements. This project provides:

- **Device Selector**: Automatically determines the most suitable quantum device for a given circuit based on metrics such as qubit count, etc.
- **Pass Selector**: Dynamically selects transpiler passes optimized for qubit state selection scenarios, improving circuit performance and reducing gate errors.

## Features

- 📊 ML-based device matching using customizable criteria
- 🧱 Modular pass selection logic, easily extendable for custom pipelines
- ⚙️ Currently compatible with Qiskit and CUDA-Q

## Installation

You can install the project dependencies using `pip`:

```bash
pip install -r requirements.txt

```

## Project Structure
```bash
mqss-selector/
├── src/
│   ├── target_predictor.py
│   ├── pass_predictor.py
│   └── mqss_selector.py
├── data/
│   ├── count_dicts/
│   ├── features/
│   ├── labels/
│   ├── qasm_circuits/
│   ├── trained_models/
├── tests/
├── notebooks/
├── requirements.txt
└── README.md
```

