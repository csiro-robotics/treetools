set(SOURCES
  treegrow.cpp
)
add_compile_options(-ggdb -O0)

ras_add_executable(treegrow
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
