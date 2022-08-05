// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#ifndef TREELIB_TREEPRUNER_H
#define TREELIB_TREEPRUNER_H

#include "treelib/treelibconfig.h"
#include <raylib/rayforeststructure.h>
#include <raylib/raytreegen.h>
#include <Eigen/Dense>

namespace tree
{
/// remove all branches which are less than the specified diameter 
void pruneDiameter(ray::ForestStructure &forest, double diameter, ray::ForestStructure &new_forest);

/// remove the specifiied length from the end of all branches
void pruneLength(ray::ForestStructure &forest, double length, ray::ForestStructure &new_forest);
}  // namespace ray

#endif  // TREELIB_TREEPRUNER_H
