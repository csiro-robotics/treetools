set(SOURCES
  treesplit.cpp
)
add_compile_options(-ggdb -O0)
ras_add_executable(treesplit
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
