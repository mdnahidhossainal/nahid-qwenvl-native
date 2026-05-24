cmake_minimum_required(VERSION 3.22)
project(nahid_qwenvl_stage5f LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT LLAMA_CPP_DIR)
    message(FATAL_ERROR "LLAMA_CPP_DIR is required")
endif()
if(NOT LLAMA_SO OR NOT GGML_SO OR NOT GGML_BASE_SO OR NOT GGML_CPU_SO)
    message(FATAL_ERROR "llama.cpp shared library paths are required")
endif()

add_library(llama SHARED IMPORTED)
set_target_properties(llama PROPERTIES IMPORTED_LOCATION "${LLAMA_SO}")
add_library(ggml SHARED IMPORTED)
set_target_properties(ggml PROPERTIES IMPORTED_LOCATION "${GGML_SO}")
add_library(ggml-base SHARED IMPORTED)
set_target_properties(ggml-base PROPERTIES IMPORTED_LOCATION "${GGML_BASE_SO}")
add_library(ggml-cpu SHARED IMPORTED)
set_target_properties(ggml-cpu PROPERTIES IMPORTED_LOCATION "${GGML_CPU_SO}")

add_library(nahid_qwenvl SHARED src/main/cpp/nahid_qwenvl.cpp)

target_include_directories(nahid_qwenvl PRIVATE
        ${LLAMA_CPP_DIR}/include
        ${LLAMA_CPP_DIR}/common
        ${LLAMA_CPP_DIR}/ggml/include
        ${LLAMA_CPP_DIR}/ggml/src
)

target_link_libraries(nahid_qwenvl
        android
        log
        llama
        ggml
        ggml-base
        ggml-cpu
)
