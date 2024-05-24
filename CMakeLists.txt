# cmake version
cmake_minimum_required(VERSION 3.15)

# project
project(LSMKV)

# cxx version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build flag
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# 启用AS
# -fsanitize=address检测内存错误
# -fno-omit-frame-pointer 保留帧指针 便于生成栈跟踪信息
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" -fsanitize=address -fno-omit-frame-pointer ASAN_OPTIONS=alloc_dealloc_mismatch=0)
# set(CMAKE_CXX_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

# compile error full
# Werror 所有警告视为错误
# Wall 启用大部分警告
# Wextra 启用额外警告
# 所有符合ISO标准的警告
# 确保严格符合C++标准
add_compile_options(-Wall )

# set executable location
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)

# add source file
set(SOURCES_FILES kvstore.cc skiplist.cc VLog.cc logger.cc)

# build executable
add_executable(correctness correctness.cc ${SOURCES_FILES})
add_executable(persistence persistence.cc ${SOURCES_FILES})
add_executable(performance performance.cc ${SOURCES_FILES})

message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")