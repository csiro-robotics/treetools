set(SOURCES
  treedecimate.cpp
)
add_compile_options(-O3)

ras_add_executable(treedecimate
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
