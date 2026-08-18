set(SOURCES
  treediff.cpp
)

add_compile_options(-ggdb -O0)
add_treetools_executable(treediff ${SOURCES})
