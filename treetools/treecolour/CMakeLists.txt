set(SOURCES
  treecolour.cpp
)

ras_add_executable(treecolour
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
