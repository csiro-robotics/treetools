// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#ifndef TREELIB_TREEUTILS_H
#define TREELIB_TREEUTILS_H

#include "treelib/treelibconfig.h"

#include <Eigen/Dense>

namespace tree
{
struct Trunk2
{
  Trunk2();
  Eigen::Vector3d centre; // height is midway up trunk
  double radius;
  double score;
  double combined_score;
  double weight;
  double thickness;
  double length; 
  struct Trunk2 *next_down;
  Eigen::Vector2d lean;
};

}  // namespace ray

#endif  // TREELIB_TREEUTILS_H
