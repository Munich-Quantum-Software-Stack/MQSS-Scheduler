# scheduler

## Description
The scheduler library offers a `scheduler` function to select a suitable device for one or multiple `QuantumTask`s (i.e. setting the `mScheduledQpu` attribute).
It also provides the `SchedulerQueue`s class which are observers of the `SubmitterQueue`s (see Submitter library).
Any scheduling algorithm only works on the `SchedulerQueue`s, which in turn determine the `mExecutionOrder` attribute of its contained `QuantumTask`s.


## Available Schedulers
- Random Forest Regressor (device selection) + Backfilling (scheduling strategy)
- Round Robin


## Getting Started

### Requirements
- ONNX Runtime (only) for Random Forest Regressor (model inference)

### Installation

1. Clone the repository:
   ```sh
   git clone https://github.com/Munich-Quantum-Software-Stack/scheduler.git
   cd scheduler
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

1. Navigate into the scheduler directory.
```sh
cd scheduler
```

2. Set the environment variables:
```sh
export QDMI_CONFIG_FILE="$(pwd)/tests/.qdmi-config"
export CONF_IBM="$(pwd)/tests/ibm_conf.json"
export PROP_IBM="$(pwd)/tests/ibm_prob.json"
```

3. Run the tests:
```sh
./build/tests/schedulerBackfillingTest
./build/tests/schedulerRoundRobinTest
```


## Contributing

1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Make your changes.
4. Commit your changes (`git commit -am 'Add new feature'`).
5. Push to the branch (`git push origin feature-branch`).
6. Create a new Pull Request.
