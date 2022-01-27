// Copyright (c) 2020
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
  std::cout << "Rotate a tree file about the origin" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treerotate treefile.txt 0,0,30  - rotation (rx,ry,rz) is a rotation vector in degrees:" << std::endl;
  std::cout << "                                  so this example rotates the file by 30 degrees in the z axis." << std::endl;
  exit(exit_code);
}

int main(int argc, char *argv[])
{
  ray::FileArgument tree_file;
  ray::Vector3dArgument rotation_arg(-360, 360);
  if (!ray::parseCommandLine(argc, argv, {&tree_file, &rotation_arg}))
    usage();
    
  Eigen::Vector3d rot = rotation_arg.value();
  const double angle = rot.norm();
  rot /= angle;
  Eigen::Quaterniond rotation(Eigen::AngleAxisd(angle * ray::kPi / 180.0, rot));

  ray::ForestStructure forest;
  forest.load(tree_file.name());
  for (auto &tree: forest.trees)
  {
    for (auto &segment: tree.segments())
      segment.tip = rotation * segment.tip;
  }
  forest.save(tree_file.name());
  
  return 0;
}
