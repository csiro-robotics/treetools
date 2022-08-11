// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#ifndef TREELIB_TREEINFORMATION_H
#define TREELIB_TREEINFORMATION_H

#include "treeutils.h"
#include "raylib/raytreestructure.h"

/// This file provides support functions for extracting tree information (used in treeinfo)
namespace tree
{
/// calculate the power law relationship for the data xs: # data larger than x = cx^d, with correlation coefficient r2
void TREELIB_EXPORT calculatePowerLaw(std::vector<double> &xs, double &c, double &d, double &r2, const std::string &graph_file = "");

/// Fill the attributes at bend_id to reflect the amount of bend in each segment of the tree trunk
void TREELIB_EXPORT setTrunkBend(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, int bend_id, int length_id);

/// Extimate how closely the specified @c tree resembles a monocot (palm) tree. Filling in the attribute at @c monocotal_id
void TREELIB_EXPORT setMonocotal(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, int monocotal_id);

}  // namespace tree

#endif  // TREELIB_TREEINFORMATION_H
