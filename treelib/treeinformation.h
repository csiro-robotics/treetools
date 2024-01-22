// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#ifndef TREELIB_TREEINFORMATION_H
#define TREELIB_TREEINFORMATION_H

#include "raylib/raytreestructure.h"
#include "treeutils.h"

/// This file provides support functions for extracting tree information (used in treeinfo)
namespace tree
{
/// calculate the power law relationship for the data xs: # data larger than x = cx^d, with correlation coefficient r2
void TREELIB_EXPORT calculatePowerLaw(std::vector<double> &xs, double &c, double &d, double &r2,
                                      const std::string &graph_file = "");

/// Fill the attributes at bend_id to reflect the amount of bend in each segment of the tree trunk
void TREELIB_EXPORT setTrunkBend(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, int bend_id,
                                 int length_id, int branch_slope_id);

/// Estimate how closely the specified @c tree resembles a monocot (palm) tree. Filling in the attribute at @c
/// monocotal_id
void TREELIB_EXPORT setMonocotal(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children,
                                 int monocotal_id);

/// Estimate the Diameter at Breast Height 
void setDBH(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, int DBH_id);

/// Estimate the branching properties: the angle, the dominance and the number of child branches
/// fill in these values into the attributes array per-segment in the tree structure, assuming these array ids are within the attribute lengths 
void TREELIB_EXPORT getBifurcationProperties(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, std::vector<double> &angles, std::vector<double> &dominances, std::vector<double> &num_children, 
  double &tree_dominance, double &tree_angle, double &total_weight);

/// set branch lengths at the branch points
void TREELIB_EXPORT getBranchLengths(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, std::vector<double> &lengths, double prune_length);
}  // namespace tree

#endif  // TREELIB_TREEINFORMATION_H
