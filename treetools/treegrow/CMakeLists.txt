set(SOURCES
  treegrow.cpp
)
add_compile_options(-O3)

ras_add_executable(treegrow
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
