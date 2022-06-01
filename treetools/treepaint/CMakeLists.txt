set(SOURCES
  treepaint.cpp
)
add_compile_options(-O3)

ras_add_executable(treepaint
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
