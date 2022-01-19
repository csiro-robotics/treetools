set(SOURCES
  treeadd.cpp
)

ras_add_executable(treeadd
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
