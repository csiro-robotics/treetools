// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#ifndef TREELIB_TREEUTILS_H
#define TREELIB_TREEUTILS_H

#include <Eigen/Dense>
#include "treelib/treelibconfig.h"

namespace tree
{
/// Basic cylinder class
struct TREELIB_EXPORT Cylinder
{
  Cylinder(const Eigen::Vector3d &v1, const Eigen::Vector3d &v2, double radius)
    : v1(v1)
    , v2(v2)
    , radius(radius)
  {}
  Eigen::Vector3d v1, v2;
  double radius;
};
/// Find the volume of intersection between two cylinders
double TREELIB_EXPORT approximateIntersectionVolume(Cylinder cyl1, Cylinder cyl2);
}  // namespace tree

#endif  // TREELIB_TREEUTILS_H
