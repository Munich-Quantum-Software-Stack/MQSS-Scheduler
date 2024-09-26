find_package(qdmi REQUIRED)
find_package(onnx REQUIRED)

target_include_directories(backfilling 
    PUBLIC 
    ${QDMI_INCLUDE_DIRS}
    ${SUBMITTER_INCLUDE_DIRS}
    ${ONNXRUNTIME_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(backfilling 
    PUBLIC 
    ${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so
    ${ONNXRUNTIME_LIB_DIR}/libonnxruntime_providers_shared.so
    submitter
    queue
)

target_sources(backfilling 
    PUBLIC
    eval.cpp
    predictor.cpp
    scheduler_backfilling.cpp
) 
