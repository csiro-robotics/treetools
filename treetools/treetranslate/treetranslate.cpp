// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "raylib/rayparse.h"
#include "raylib/rayforestgen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

void usage(int exit_code = 1)
{
  std::cout << "Translate a tree file" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treetranslate treefile.txt 0,0,1 - translation (x,y,z) in metres" << std::endl;
  exit(exit_code);
}


int main(int argc, char *argv[])
{
  ray::FileArgument tree_file;
  ray::Vector3dArgument translation3;

  bool vec3 = ray::parseCommandLine(argc, argv, {&tree_file, &translation3});
  if (!vec3)
    usage();

  Eigen::Vector3d translation = translation3.value();

  ray::ForestStructure forest;
  forest.load(tree_file.name());
  for (auto &tree: forest.trees)
  {
    for (auto &segment: tree.segments())
      segment.tip += translation;
  }
  forest.save(tree_file.name());

  return 0;
}
