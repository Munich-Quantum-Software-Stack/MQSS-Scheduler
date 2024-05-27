include(FetchContent)

FetchContent_Declare(
    submitter
    #GIT_REPOSITORY git@github.com:Munich-Quantum-Software-Stack/submitter.git
    #GIT_TAG patrick/update
    SOURCE_DIR /home/ubuntu/mqss/submitter
)

FetchContent_MakeAvailable(submitter)