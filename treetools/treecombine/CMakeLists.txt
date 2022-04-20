set(SOURCES
  treecombine.cpp
)
add_compile_options(-ggdb -O0)

ras_add_executable(treecombine
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
