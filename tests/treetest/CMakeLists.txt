# Copyright (c) 2026
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230

set(SOURCES
  environment.cpp
  treetests.cpp
  testmain.cpp
)
add_executable(treetest ${SOURCES})
target_include_directories(treetest
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/treelib>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)
target_include_directories(treetest SYSTEM
  PRIVATE
    "${GTEST_INCLUDE_DIRS}"
)
target_link_libraries(treetest
  PUBLIC
    treelib
    GTest::Main
)
target_link_libraries(treetest
  PRIVATE
    Eigen3::Eigen
)
gtest_discover_tests(treetest
  EXTRA_ARGS ${CMAKE_BINARY_DIR}
)
