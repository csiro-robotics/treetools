// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treeutils.h"
#include <raylib/rayutils.h>

namespace tree
{
// a tough function, so we will make a 'similar-axes' approximation
double intersectionVolume(Cylinder cyl1, Cylinder cyl2)
{
  Eigen::Vector3d dir1 = cyl1.v2 - cyl1.v1;
  Eigen::Vector3d dir2 = cyl2.v2 - cyl2.v1;
  double r = cyl1.radius;
  double R = cyl2.radius;

  // start with a capsule-capsule exclusion test that doesn't assume similar axes
  Eigen::Vector3d cr = dir1.cross(dir2);
  Eigen::Vector3d side1 = cr.cross(dir1);
  Eigen::Vector3d side2 = cr.cross(dir2);
  double den1 = dir1.dot(side2);
  double den2 = dir2.dot(side1);
  const double eps = 1e-6;
  // capsule intersection doesn't work if they are parallel, but in that case we don't need this test
  if (std::abs(den1) > eps && std::abs(den2) > eps)
  {
    double f1 = -(cyl1.v1 - cyl2.v1).dot(side2) / den1;
    Eigen::Vector3d p1 = cyl1.v1 + dir1 * std::max(0.0, std::min(f1, 1.0));
    double f2 = -(cyl2.v1 - cyl1.v1).dot(side1) / den2;
    Eigen::Vector3d p2 = cyl2.v1 + dir2 * std::max(0.0, std::min(f2, 1.0));
    double distance_sqr = (p1 - p2).squaredNorm();
    if (distance_sqr >= (R + r) * (R + r))
      return 0.0;
  }

  if (dir2.dot(dir1) < 0.0)
  {
    dir2 *= -1.0;
    std::swap(cyl2.v1, cyl2.v2);
  }
  Eigen::Vector3d dir = (dir1 + dir2).normalized();
  double d1 = cyl1.v1.dot(dir);
  double d2 = cyl1.v2.dot(dir);
  double e1 = cyl2.v1.dot(dir);
  double e2 = cyl2.v2.dot(dir);
  double mind = std::min(d1, d2);
  double maxd = std::max(d1, d2);
  double mine = std::min(e1, e2);
  double maxe = std::max(e1, e2);
  if (mind >= maxe || mine >= maxd)  // they don't overlap along the similar axis
    return 0.0;

  double minx = std::min(maxd, maxe);
  double maxx = std::max(mind, mine);
  double overlap_length = minx - maxx;
  if (overlap_length <= 0.0)
    return 0.0;
  if (!(overlap_length == overlap_length))
    std::cout << "bad overlap_length " << overlap_length << std::endl;

  double midD = (minx + maxx) / 2.0;
  Eigen::Vector3d pos1 = cyl1.v1 + (cyl1.v2 - cyl1.v1) * (midD - d1) / (d2 - d1);
  Eigen::Vector3d pos2 = cyl2.v1 + (cyl2.v2 - cyl2.v1) * (midD - e1) / (e2 - e1);
  double d = (pos1 - pos2).norm();
  if (d >= r + R)
    return 0.0;
  double minR = std::min(r, R);
  double maxR = std::max(r, R);
  if (d < 1e-6 + maxR - minR)
  {
    double area = ray::kPi * minR * minR;
    return area * overlap_length;
  }

  // from: https://mathworld.wolfram.com/Circle-CircleIntersection.html
  double cos1 = (d * d + r * r - R * R) / (2.0 * d * r);
  double cos2 = (d * d + R * R - r * r) / (2.0 * d * R);
  double square = (-d + r + R) * (d + r - R) * (d - r + R) * (d + r + R);
  double area = r * r * std::acos(cos1) + R * R * std::acos(cos2) - 0.5 * std::sqrt(square);
  if (!(area == area))
    std::cout << "bad area " << area << std::endl;
  return area * overlap_length;
}

}  // namespace tree
