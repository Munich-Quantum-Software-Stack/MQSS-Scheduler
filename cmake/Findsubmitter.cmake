include(FetchContent)

FetchContent_Declare(
    submitter
    GIT_REPOSITORY git@github.com:Munich-Quantum-Software-Stack/submitter.git
    GIT_TAG develop
)

FetchContent_MakeAvailable(submitter)
