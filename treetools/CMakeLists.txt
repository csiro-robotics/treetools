# Copyright (c) 2019
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230
#
# Author: Rod Taylor

function(add_treetools_executable EXEC_NAME EXEC_SOURCES_LIST)
  add_executable(${EXEC_NAME} ${EXEC_SOURCES_LIST})
  target_link_libraries(${EXEC_NAME} PRIVATE treelib)
  set_target_properties(${EXEC_NAME} PROPERTIES DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")
  set_target_properties(${EXEC_NAME} PROPERTIES VERSION ${${CMAKE_PROJECT_NAME}_VERSION})
  install(TARGETS ${EXEC_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION "${INCLUDE_INSTALL_DIR}"
  )
endfunction()

add_subdirectory(treecolour)
add_subdirectory(treecombine)
add_subdirectory(treecreate)
add_subdirectory(treedecimate)
add_subdirectory(treediff)
add_subdirectory(treefoliage)
add_subdirectory(treegrow)
add_subdirectory(treeinfo)
add_subdirectory(treemesh)
add_subdirectory(treepaint)
add_subdirectory(treeprune)
add_subdirectory(treerender)
add_subdirectory(treerotate)
add_subdirectory(treesmooth)
add_subdirectory(treesplit)
add_subdirectory(treetranslate)
