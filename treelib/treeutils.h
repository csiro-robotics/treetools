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
struct Cylinder
{
  Cylinder(const Eigen::Vector3d &v1, const Eigen::Vector3d &v2, double radius) : v1(v1), v2(v2), radius(radius) {}
  Eigen::Vector3d v1, v2;
  double radius;
};
double intersectionVolume(Cylinder cyl1, Cylinder cyl2);
}  // namespace ray

#endif  // TREELIB_TREEUTILS_H
