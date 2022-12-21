set(SOURCES
  treemesh.cpp
)

ras_add_executable(treemesh
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
