# Copyright (c) 2022
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230
#
# Author: Thomas Lowe

# Setup configuration header.
configure_file(treelibconfig.in.h "${CMAKE_CURRENT_BINARY_DIR}/treelibconfig.h")

set(PUBLIC_HEADERS
  treeinformation.h
  treepruner.h
  treeutils.h
)

set(PRIVATE_HEADERS
  imageread.h
  imagewrite.h
)

set(SOURCES
  ${PUBLIC_HEADERS}
  treeinformation.cpp
  treepruner.cpp
  treeutils.cpp
)

add_compile_options(-fPIC)

add_library(treelib SHARED ${SOURCES})

include(GenerateExportHeader)
generate_export_header(treelib)

target_compile_options(treelib PUBLIC ${OpenMP_CXX_FLAGS})
target_include_directories(treelib PUBLIC
  ${TREETOOLS_INCLUDE}
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(treelib PUBLIC ${TREETOOLS_LINK})

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/treelib_export.h"
    "${CMAKE_CURRENT_BINARY_DIR}/treelibconfig.h"
  DESTINATION include/treelib)

foreach(PUBLIC_HEADER ${PUBLIC_HEADERS})
  get_filename_component(PUBLIC_HEADER_DIRECTORY ${PUBLIC_HEADER} DIRECTORY)
  install(FILES ${PUBLIC_HEADER} DESTINATION include/treelib/${PUBLIC_HEADER_DIRECTORY})
endforeach()

install(TARGETS treelib
  EXPORT ${CMAKE_PROJECT_NAME}-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include/treelib
)
