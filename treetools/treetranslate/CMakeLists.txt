set(SOURCES
  treetranslate.cpp
)

ras_add_executable(treetranslate
  LIBS treelib
  SOURCES ${SOURCES}
  PROJECT_FOLDER "treetools"
)
