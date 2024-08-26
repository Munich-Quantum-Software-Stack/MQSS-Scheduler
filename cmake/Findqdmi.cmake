include(FetchContent)

FetchContent_Declare(
    qdmi
    GIT_REPOSITORY git@github.com:Munich-Quantum-Software-Stack/qdmi.git
    GIT_TAG issue/17
)

FetchContent_MakeAvailable(qdmi)

FetchContent_GetProperties(qdmi)

set(QDMI_INCLUDE_DIRS "${submitter_SOURCE_DIR}/include")
