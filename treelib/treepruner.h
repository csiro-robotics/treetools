// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#ifndef TREELIB_TREEPRUNER_H
#define TREELIB_TREEPRUNER_H

#include <raylib/rayforeststructure.h>
#include <raylib/raytreegen.h>
#include <Eigen/Dense>
#include "treelib/treelibconfig.h"

namespace tree
{
/// remove all branches which are less than the specified diameter
void TREELIB_EXPORT pruneDiameter(ray::ForestStructure &forest, double diameter, ray::ForestStructure &new_forest);

/// remove the specifiied length from the end of all branches
void TREELIB_EXPORT pruneLength(ray::ForestStructure &forest, double length, ray::ForestStructure &new_forest);
}  // namespace tree

#endif  // TREELIB_TREEPRUNER_H
