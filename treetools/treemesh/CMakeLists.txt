set(SOURCES
  treemesh.cpp
)
add_compile_options(-ggdb -O0)

ras_add_executable(treemesh
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
