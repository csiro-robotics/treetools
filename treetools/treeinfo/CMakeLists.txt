set(SOURCES
  treeinfo.cpp
)

ras_add_executable(treeinfo
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
