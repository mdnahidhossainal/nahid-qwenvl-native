cmake_minimum_required(VERSION 3.22.1)
project(nahid_qwenvl_stage5j)

add_library(nahid_qwenvl SHARED src/main/cpp/nahid_qwenvl.cpp)

find_library(log-lib log)
find_library(dl-lib dl)

target_compile_features(nahid_qwenvl PRIVATE cxx_std_17)
target_link_libraries(nahid_qwenvl ${log-lib} ${dl-lib})
