set(SOURCES
  treefoliage.cpp
)

ras_add_executable(treefoliage
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
