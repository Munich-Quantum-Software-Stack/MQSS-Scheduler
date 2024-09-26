find_package(qdmi REQUIRED)
find_package(submitter REQUIRED)

target_include_directories(round_robin 
    PUBLIC 
    SUBMITTER_INCLUDE_DIRS
) 

target_link_libraries(round_robin 
    PUBLIC 
    qdmi 
    qinfo 
    LLVM 
    submitter 
    queue
)

target_sources(round_robin 
    PRIVATE 
    scheduler_round_robin.cpp
)
