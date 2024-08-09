add_library(setup 
    setup.cpp
)

target_link_libraries(setup PRIVATE
    queue
    LLVM
)

target_include_directories(setup PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${QDMI_INCLUDE_DIRS}
    ${SUBMITTER_INCLUDE_DIRS}
)