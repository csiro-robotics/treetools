# Setup of GTEST changed at CMake 3.5.
cmake_minimum_required(VERSION 3.5)

find_package(GTest)
# Eigen required to support some tests - NDT in particular
find_package(Eigen3 QUIET)

set(SOURCES
  treetests.cpp
  testmain.cpp
)

add_executable(treetest ${SOURCES})
set_target_properties(treetest PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
set_target_properties(treetest PROPERTIES FOLDER tests)

target_include_directories(treetest
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/treelib>    
)

target_include_directories(treetest SYSTEM
  PRIVATE
    "${GTEST_INCLUDE_DIRS}"
)

target_link_libraries(treetest PUBLIC treelib GTest::Main)

if(Eigen3_FOUND)
  target_link_libraries(treetest PRIVATE Eigen3::Eigen)
endif(Eigen3_FOUND)

add_test(NAME treetest COMMAND treetest --gtest_output=xml:test-reports/
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

source_group("source" REGULAR_EXPRESSION ".*$")
