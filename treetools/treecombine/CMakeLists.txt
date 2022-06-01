set(SOURCES
  treecombine.cpp
)
add_compile_options(-O3)

ras_add_executable(treecombine
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
