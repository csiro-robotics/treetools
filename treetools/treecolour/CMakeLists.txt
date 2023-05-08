set(SOURCES
  treecolour.cpp
)
add_compile_options(-O0 -ggdb)

ras_add_executable(treecolour
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
