set(SOURCES
  treeprune.cpp
)

ras_add_executable(treeprune
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
