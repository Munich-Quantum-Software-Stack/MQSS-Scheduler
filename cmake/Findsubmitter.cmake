include(FetchContent)

FetchContent_Declare(
    submitter
    GIT_REPOSITORY git@github.com:Munich-Quantum-Software-Stack/submitter.git
    GIT_TAG patrick/update
)

FetchContent_MakeAvailable(submitter)