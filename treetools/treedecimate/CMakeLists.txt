set(SOURCES
  treedecimate.cpp
)

ras_add_executable(treedecimate
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
