set(SOURCES
  treesplit.cpp
)

ras_add_executable(treesplit
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
