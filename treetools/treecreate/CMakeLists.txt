set(SOURCES
  treecreate.cpp
)

ras_add_executable(treecreate
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
