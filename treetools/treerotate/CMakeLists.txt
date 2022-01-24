set(SOURCES
  treerotate.cpp
)

ras_add_executable(treerotate
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
