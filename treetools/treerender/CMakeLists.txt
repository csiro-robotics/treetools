set(SOURCES
  treerender.cpp
)
ras_add_executable(treerender
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
