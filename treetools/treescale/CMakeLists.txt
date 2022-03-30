set(SOURCES
  treescale.cpp
)
add_compile_options(-ggdb -O0)

ras_add_executable(treescale
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
