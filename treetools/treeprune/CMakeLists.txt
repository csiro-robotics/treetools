set(SOURCES
  treeprune.cpp
)
add_compile_options(-O3)

ras_add_executable(treeprune
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
