set(SOURCES
  treediff.cpp
)

ras_add_executable(treediff
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
