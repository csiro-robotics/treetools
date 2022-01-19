# Copyright (c) 2022
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230
#
# Author: Thomas Lowe

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  message(STATUS "Build type empty, so defaulting to Release.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# Setup configuration header
configure_file(treelibconfig.in.h "${CMAKE_CURRENT_BINARY_DIR}/treelibconfig.h")

set(PUBLIC_HEADERS
  treeutils.h
)

set(SOURCES
  ${PUBLIC_HEADERS}
  treeutils.cpp
)

ras_add_library(treelib
  TYPE SHARED
  INCLUDE_PREFIX "treelib"
  PROJECT_FOLDER "treelib"
  INCLUDE
    PUBLIC_SYSTEM
      ${TREETOOLS_INCLUDE}
  LIBS
    PUBLIC
      ${TREETOOLS_LINK}
  PUBLIC_HEADERS ${PUBLIC_HEADERS}
  GENERATED PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/treelibconfig.h"
  SOURCES ${SOURCES}
)

target_compile_options(treelib PUBLIC ${OpenMP_CXX_FLAGS})
