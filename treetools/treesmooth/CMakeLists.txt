set(SOURCES
  treesmooth.cpp
)

ras_add_executable(treesmooth
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
