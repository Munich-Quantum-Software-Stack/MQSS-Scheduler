# submitter

## Description
The submitter library provides the `Submitter` and `QuantumTask` classes.
A `Submitter` instance is associated with a QDMI device and will keep it busy by submitting `QuantumTask`s from its queue.


## Submission strategy
- The devices are kept busy at all times (with a fixed max. number of `QuantumTask`s)
- Tasks stay in submitter queue as long as possible to account for changes in execution order by the scheduler


## Getting Started

### Requirements
- ...

### Installation

1. Clone the repository:
   ```sh
   git clone https://github.com/Munich-Quantum-Software-Stack/submitter.git
   cd submitter
   ```

2. Create a build directory and navigate into it:
   ```sh
   mkdir build
   cd build
   ```

3. Run CMake to configure the project:
   ```sh
   cmake ..
   ```

4. Build the project:
   ```sh
   make
   ```

### Running Tests
After the project was built successfully.

1. Navigate into the submitter directory.
```sh
cd submitter
```

2. Set the environment variables:
```sh
export QDMI_CONFIG_FILE="$(pwd)/test/config/.qdmi-config"
export CONF_IBM="$(pwd)/test/config/ibm_conf.json"
export PROP_IBM="$(pwd)/test/config/ibm_prob.json"
```

3. Run the test:
```sh
./build/test/submitterTest
```


## Contributing

1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Make your changes.
4. Commit your changes (`git commit -am 'Add new feature'`).
5. Push to the branch (`git push origin feature-branch`).
6. Create a new Pull Request.
