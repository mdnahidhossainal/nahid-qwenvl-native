cmake_minimum_required(VERSION 3.22.1)
project(nahid_qwenvl_stage5l)

if(NOT LLAMA_CPP_DIR)
    message(FATAL_ERROR "LLAMA_CPP_DIR is required")
endif()

# Locate mtmd.h dynamically because its folder can move between llama.cpp versions.
file(GLOB_RECURSE MTMD_HEADER_CANDIDATES "${LLAMA_CPP_DIR}/*/mtmd.h")
if(NOT MTMD_HEADER_CANDIDATES)
    message(FATAL_ERROR "mtmd.h not found inside LLAMA_CPP_DIR=${LLAMA_CPP_DIR}")
endif()
list(GET MTMD_HEADER_CANDIDATES 0 MTMD_HEADER_PATH)
get_filename_component(MTMD_HEADER_DIR "${MTMD_HEADER_PATH}" DIRECTORY)
message(STATUS "Using mtmd header: ${MTMD_HEADER_PATH}")

add_library(nahid_qwenvl SHARED src/main/cpp/nahid_qwenvl.cpp)

find_library(log-lib log)
find_library(dl-lib dl)
find_library(android-lib android)

target_compile_features(nahid_qwenvl PRIVATE cxx_std_17)
target_include_directories(nahid_qwenvl PRIVATE
    ${LLAMA_CPP_DIR}/include
    ${LLAMA_CPP_DIR}/common
    ${LLAMA_CPP_DIR}/ggml/include
    ${LLAMA_CPP_DIR}/ggml/src
    ${MTMD_HEADER_DIR}
)

target_link_libraries(nahid_qwenvl ${log-lib} ${dl-lib} ${android-lib})
