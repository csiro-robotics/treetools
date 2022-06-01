set(SOURCES
  treefoliage.cpp
)
add_compile_options(-O3)

ras_add_executable(treefoliage
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
