add_library(backend_test SHARED)

target_sources(backend_test PRIVATE test_backend.c)

target_include_directories(backend_test PUBLIC
    ${QDMI_SRC_DIRS}
)

target_link_libraries(backend_test PUBLIC qdmi)
