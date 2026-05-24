cmake_minimum_required(VERSION 3.18.1)

project(nahid_qwenvl LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nahid_qwenvl SHARED
    src/main/cpp/nahid_qwenvl.cpp
)

find_library(log-lib log)
find_library(android-lib android)
find_library(dl-lib dl)

target_link_libraries(nahid_qwenvl
    ${log-lib}
    ${android-lib}
    ${dl-lib}
)
