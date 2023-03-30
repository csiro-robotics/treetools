set(SOURCES
  treecombine.cpp
)
ras_add_executable(treecombine
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
