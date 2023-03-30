// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "raylib/rayforeststructure.h"
#include "raylib/rayparse.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Translate a tree file" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treetranslate treefile.txt 0,0,1 - translation (x,y,z) in metres" << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method translates a tree file in-place. To undo, apply the negative translation.
int main(int argc, char *argv[])
{
  ray::FileArgument tree_file;
  ray::Vector3dArgument translation3;

  if (!ray::parseCommandLine(argc, argv, { &tree_file, &translation3 }))
  {
    usage();
  }

  const Eigen::Vector3d translation = translation3.value();

  ray::ForestStructure forest;
  forest.load(tree_file.name());
  // translation requires just the tip parameter to be updated
  for (auto &tree : forest.trees)
  {
    for (auto &segment : tree.segments())
    {
      segment.tip += translation;
    }
  }
  forest.save(tree_file.name());

  return 0;
}
