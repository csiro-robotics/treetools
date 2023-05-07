set(SOURCES
  treegrow.cpp
)
add_compile_options(-O0 -ggdb)

ras_add_executable(treegrow
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
