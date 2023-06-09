// Copyright (c) 2020
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "raylib/rayforeststructure.h"
#include "raylib/rayparse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Rotate a tree file about the origin" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treerotate treefile.txt 0,0,30  - rotation (rx,ry,rz) is a rotation vector in degrees:" << std::endl;
  std::cout << "                                  so this example rotates the file by 30 degrees in the z axis."
            << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method rotates the tree cloud in-place. To undo, apply the negative rotation.
int main(int argc, char *argv[])
{
  ray::FileArgument tree_file;
  ray::Vector3dArgument rotation_arg(-360, 360);
  if (!ray::parseCommandLine(argc, argv, { &tree_file, &rotation_arg }))
  {
    usage();
  }

  // convert the rotation argument into a quaternion
  Eigen::Vector3d rot = rotation_arg.value();
  const double angle = rot.norm();
  rot /= angle;
  const Eigen::Quaterniond rotation(Eigen::AngleAxisd(angle * ray::kPi / 180.0, rot));

  ray::ForestStructure forest;
  forest.load(tree_file.name());
  // rotate the tips, that is all that needs rotating
  for (auto &tree : forest.trees)
  {
    for (auto &segment : tree.segments())
    {
      segment.tip = rotation * segment.tip;
    }
  }
  forest.save(tree_file.name());

  return 0;
}
