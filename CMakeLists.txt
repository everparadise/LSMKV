cmake_minimum_required(VERSION 3.15)
project(correctness)
add_executable(correctness correctness.cc kvstore.cc skiplist.cc VLog.cc)
set(CMAKE_BUILD_TYPE Debug)