# QC Training Package

This package provides tools for preparing data and training models on labeled quantum circuits. It consists of two main modules: `setup.py` and `training.py`, along with the corresponding test files `test_training.py`and `test_setup.py`.

## Description

- **setup.py**: This module provides functions to prepare test and example data for the `training.py` script.
- **training.py**: This module provides functions to calculate circuit feature dictionaries and a training routine to train a `RandomForestRegressor` model on the extracted features.

## Installation

To install the package, clone the repository and navigate to the project directory:

```sh
git clone https://github.com/Munich-Quantum-Software-Stack/scheduler.git
cd predictor
```

Then, install the package using pip:

```sh
pip install .
```

## Usage

### Preparing Data

To get an impression of the required files, you can run the example.py script:
    
```sh
python src/example.py
```

### Directory Structure

The package expects the following directory structure:

```sh
predictor/
|-- data/
|   |-- experiment_name/
|   |   |-- circuits/   # QASM circuits (required for training)
|   |   |-- features/   
|   |   |-- labels/     # .pkl labels (required for training)
|-- src/
|   |-- setup.py
|   |-- training.py
|-- tests/
|   |-- test_setup.py
|   |-- test_training.py
```

### Training

To execute training for a specific experiment, run the following command:
    
```sh
python src/training.py --experiment <experiment_name>
```

## Testing

To run the tests, execute the following command:

```sh
pytest tests/test_setup.py
pytest tests/test_training.py
```