# QC Training Package

This package provides tools for preparing data and training models on labeled quantum circuits. It consists of two main modules: `utils.py` and `training.py`, along with the corresponding test files `test_training.py` and `test_utils.py`.

## Description

- **utils.py**: This module provides functions to prepare test and example data for the `training.py` script.
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

To get an impression of the required files, run the following command:

```sh
prepare-data --experiment-name example
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
|   |-- utils.py
|   |-- training.py
|-- tests/
|   |-- test_utils.py
|   |-- test_training.py
```

### Training

To execute training for a specific experiment, run the following command:

```sh
train-model --experiment-name <experiment_name>
```

After running the setup example you can use:

```sh
train-model --experiment-name example
```

## Testing

To run the tests, execute the following commands:

```sh
pytest tests/test_utils.py
pytest tests/test_training.py
```

## Contributing

Contributions are welcome! Please open an issue first to discuss what you would like to change.
Once you started working on your pull request, please make sure to use linting and formatting through:

```sh
ruff check --fix
```
