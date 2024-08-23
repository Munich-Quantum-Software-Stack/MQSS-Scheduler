add_library(test_backend_1 SHARED)
add_library(test_backend_2 SHARED)

target_sources(test_backend_1 PRIVATE test_backend_1.c)
target_sources(test_backend_2 PRIVATE test_backend_2.c)

target_include_directories(test_backend_1 PUBLIC
    ${QDMI_SRC_DIRS}
)
target_include_directories(test_backend_2 PUBLIC
    ${QDMI_SRC_DIRS}
)

target_link_libraries(test_backend_1 PUBLIC qdmi)
target_link_libraries(test_backend_2 PUBLIC qdmi)
