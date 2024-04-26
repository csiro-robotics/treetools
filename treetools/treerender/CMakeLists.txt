set(SOURCES
  treerender.cpp
)
add_compile_options(-ggdb -O0)
ras_add_executable(treerender
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
