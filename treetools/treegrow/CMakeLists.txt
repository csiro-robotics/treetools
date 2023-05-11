set(SOURCES
  treegrow.cpp
)

ras_add_executable(treegrow
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
