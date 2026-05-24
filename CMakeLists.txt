cmake_minimum_required(VERSION 3.22.1)
project(nahid_qwenvl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nahid_qwenvl SHARED
    src/main/cpp/nahid_qwenvl.cpp
)

target_include_directories(nahid_qwenvl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(nahid_qwenvl
    ${log-lib}
    ${android-lib}
    dl
)
