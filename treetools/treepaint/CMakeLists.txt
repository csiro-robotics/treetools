set(SOURCES
  treepaint.cpp
)
add_compile_options(-ggdb -O0)

ras_add_executable(treepaint
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
