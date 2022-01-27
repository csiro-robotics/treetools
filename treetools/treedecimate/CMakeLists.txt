set(SOURCES
  treedecimate.cpp
)
add_compile_options(-ggdb -O0)

ras_add_executable(treedecimate
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
