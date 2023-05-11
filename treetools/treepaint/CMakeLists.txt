set(SOURCES
  treepaint.cpp
)

ras_add_executable(treepaint
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
