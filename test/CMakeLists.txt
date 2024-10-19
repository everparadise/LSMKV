# include Google Test or any other test framework
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCES_FILES 
    ${PROJECT_SOURCE_DIR}/src/code/kvstore.cc 
    ${PROJECT_SOURCE_DIR}/src/code/skiplist.cc 
    ${PROJECT_SOURCE_DIR}/src/code/VLog.cc 
    ${PROJECT_SOURCE_DIR}/src/code/logger.cc
)

# 添加测试源文件
add_executable(unit_tests demo.cpp ${SOURCES_FILES})

target_link_libraries(unit_tests ${GTEST_LIBRARIES} gtest_main)
