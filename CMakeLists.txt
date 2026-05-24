cmake_minimum_required(VERSION 3.22.1)
project(nahid_qwenvl)

add_library(nahid_qwenvl SHARED
    src/main/cpp/nahid_qwenvl.cpp
)

find_library(log-lib log)
target_link_libraries(nahid_qwenvl ${log-lib})
