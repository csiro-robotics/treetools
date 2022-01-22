set(SOURCES
  treefoliage.cpp
)
add_compile_options(-ggdb -O0)

ras_add_executable(treefoliage
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
