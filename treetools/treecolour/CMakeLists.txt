set(SOURCES
  treecolour.cpp
)
add_compile_options(-ggdb -O0)

ras_add_executable(treecolour
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
