set(SOURCES
  treeinfo.cpp
)
add_compile_options(-ggdb -O0)
ras_add_executable(treeinfo
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
