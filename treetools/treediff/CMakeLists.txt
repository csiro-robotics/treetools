set(SOURCES
  treediff.cpp
)
add_compile_options(-ggdb -O0)
ras_add_executable(treediff
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
