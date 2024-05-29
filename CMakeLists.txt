# Copyright (c) 2022
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230
#
# Author: Thomas Lowe
cmake_minimum_required(VERSION 3.10)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  message(STATUS "Build type empty, so defaulting to Release.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# Setup project details.
project(treetools)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(RasProjectSetup)
ras_project(
  VERSION 0.0.1
  NAMESPACE tree
)

# Add project level options here.
# Setup doxygen
option(TREE_BUILD_DOXYGEN "Build doxgen documentation?" OFF)
# Setup unit tests
option(TREE_BUILD_TESTS "Build unit tests?" OFF)
option(WITH_TIFF "With libgeotiff support?" OFF)
ras_bool_to_int(WITH_TIFF)

if(WITH_TIFF)
  find_package(PROJ REQUIRED COMPONENTS proj)
endif(WITH_TIFF)

# Required packages.
find_package(raycloudtools REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(libnabo REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Threads)

set(TREETOOLS_INCLUDE ${EIGEN3_INCLUDE_DIRS} ${libnabo_INCLUDE_DIRS} ${raycloudtools_INCLUDE_DIRS})
set(TREETOOLS_LINK ${libnabo_LIBRARIES} raycloud::raylib Threads::Threads)



message(STATUS "TREETOOLS_INCLUDE: ${TREETOOLS_INCLUDE}")
message(STATUS "TREETOOLS_LINK: ${TREETOOLS_LINK}")

# Create libs
add_subdirectory(treelib)
add_subdirectory(treetools)

# Test setup.
if(TREE_BUILD_TESTS)
  find_package(GTest REQUIRED)

  # We can enable testing here and/or in the subdirectory, but doing it here allows us to run CTest from the build root.
  # To run the tests, we execute:
  #   CTest -C [Debug|Release|RelWithDebInfo|MinSizeRel] --output-on-failure
  # CTest normally shows only a very terse test ouput, but we make sure failed tests show all output by adding
  #   --output-on-failure
  # The full test output is always available in:
  #   <build>/Testing/Temporary/LastTest.log
  enable_testing()
  add_subdirectory(tests)
endif(TREE_BUILD_TESTS)

# Doxygen setup.
if(TREE_BUILD_DOXYGEN)
  # Include Doxygen helper functions. This also finds the Doxygen package.
  include(RasDoxygen)

  if(DOXYGEN_FOUND)
    # Create a target to build the documentation.
    # Here we also setup various documentation variables passed through to the doxyfile configuration.
    # Each named argument below describes the Doxygen variable it sets.
    ras_doxygen_create(
      # DOXYFILE cmake/doxyfile.in  # Doxyfile to configure.
      PROJECT ${CMAKE_PROJECT_NAME} # PROJECT_NAME
      VERSION ${treetools_VERSION}   # PROJECT_NUMBER
      OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/html # HTML_OUPTUT
      # CSS <style>.css             # HTML_STYLESHEET
      PUBLISHER "CSIRO"             # DOCSET_PUBLISHER_NAME
      PUBLISHER_ID au.csiro         # DOCSET_PUBLISHER_ID
      PROJECT_ID au.csiro.ras  # DOCSET_BUNDLE_ID, QHP_NAMESPACE, ECLIPSE_DOC_ID
      PATHS                         # INPUT (RECURSIVE is on)
        src
        doc
      EXCLUDE_PATHS                 # EXCLUDE
      # Where to find source code examples.
      # EXAMPLE_PATHS <paths>        # EXAMPLE_PATH
      # Where to find images.
      # IMAGE_PATHS <paths>          # IMAGE_PATH
    )

    # Setup installation of the generated documentation: source, destination.
    ras_doxygen_install(${CMAKE_CURRENT_BINARY_DIR}/html ras)
  else(DOXYGEN_FOUND)
    message(FATAL_ERROR "Told to build Doxygen Documentation, but failed to find Doxygen")
  endif(DOXYGEN_FOUND)
endif(TREE_BUILD_DOXYGEN)

# Installation
#include(InstallRequiredSystemLibraries)
#include(CMakePackageConfigHelpers)

#install(EXPORT ${CMAKE_PROJECT_NAME}-targets
#  FILE ${CMAKE_PROJECT_NAME}-targets.cmake
#  NAMESPACE ${PACKAGE_NAMESPACE}
#  DESTINATION ${PACKAGE_EXPORT_LOCATION}
#)

#write_basic_package_version_file(
#  "${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-version.cmake"
#  VERSION ${treetools_VERSION}
#  COMPATIBILITY SameMajorVersion
#)

#configure_package_config_file(
#    "cmake/Config.in.cmake"
#    "${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake"
#    INSTALL_DESTINATION "${PACKAGE_EXPORT_LOCATION}"
#)

## Install package files
#install(FILES
#  "${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake"
#  "${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-version.cmake"
#  DESTINATION "${PACKAGE_EXPORT_LOCATION}")
