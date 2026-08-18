# Copyright (c) 2022
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230
#
# Author: Thomas Lowe

cmake_minimum_required(VERSION 3.10)

# Setup project details.
project(treetools VERSION 0.0.1
  DESCRIPTION "Command line tools for manipulating geometrical descriptions of forests. Used to analyse and process the forest reconstructions primarily from raycloudtools."
  LANGUAGES CXX
)

# Add project level options here.
option(TREE_BUILD_TESTS "Build unit tests?" OFF)

# Setup CMake options.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Setup compiler warnings and options.
add_compile_options(
  "-pedantic"
  "-Wall"
  "-Wextra"
  "-Wconversion"
  "-Werror=pedantic"
  "-Werror=vla"
  "-fdiagnostics-color=always"
)

# Required packages.
find_package(raycloudtools REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(libnabo REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)

set(TREETOOLS_INCLUDE
  ${EIGEN3_INCLUDE_DIRS}
  ${libnabo_INCLUDE_DIRS}
  ${raycloudtools_INCLUDE_DIRS}
)
set(TREETOOLS_LINK
  ${libnabo_LIBRARIES}
  raycloud::raylib
  Threads::Threads
)

# Create libraries and executables.
add_subdirectory(treelib)
add_subdirectory(treetools)

# Export targets.
include(CMakePackageConfigHelpers)
install(
  EXPORT "${CMAKE_PROJECT_NAME}-targets"
  DESTINATION "lib/cmake/${CMAKE_PROJECT_NAME}"
  NAMESPACE "tree::"
)
configure_package_config_file(
  "cmake/Config.in.cmake"
  "${PROJECT_BINARY_DIR}/config.cmake"
  INSTALL_DESTINATION "lib/cmake/${CMAKE_PROJECT_NAME}"
)
install(
  FILES
    "${PROJECT_BINARY_DIR}/config.cmake"
  DESTINATION "lib/cmake/${CMAKE_PROJECT_NAME}"
  RENAME "${CMAKE_PROJECT_NAME}-config.cmake"
)

# Test setup.
if(TREE_BUILD_TESTS)
  find_package(GTest REQUIRED)
  enable_testing()
  add_subdirectory(tests)
endif(TREE_BUILD_TESTS)
