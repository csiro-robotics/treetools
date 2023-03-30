// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treepruner.h"
#include <raylib/rayutils.h>

namespace tree
{
/// remove all branches which are less than the specified diameter
void pruneDiameter(ray::ForestStructure &forest, double diameter_value, ray::ForestStructure &new_forest)
{
  new_forest = forest;

  for (int t = 0; t < static_cast<int>(forest.trees.size()); t++)
  {
    auto &tree = forest.trees[t];
    // temporarily get the children list, it helps
    std::vector<std::vector<int>> children(tree.segments().size());
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      children[tree.segments()[i].parent_id].push_back(static_cast<int>(i));
    }
    // firstly, get the maximum diameter for each section to its end.
    // this data is monotonically decreasing, so easier to work with
    std::vector<double> max_diameter(tree.segments().size(), 0);
    for (size_t i = 0; i < tree.segments().size(); i++)
    {
      if (children[i].size() == 0)  // a leaf, so ...
      {
        int parent = tree.segments()[i].parent_id;
        int child = static_cast<int>(i);
        max_diameter[child] = 2.0 * tree.segments()[child].radius;

        while (parent != -1)  // from the leaf, iterate through its parents
        {
          double diameter_parent = 2.0 * tree.segments()[parent].radius;
          double diameter = std::max(max_diameter[child], diameter_parent);
          if (diameter > max_diameter[parent])
          {
            max_diameter[parent] = diameter;
          }
          else
          {
            break;
          }
          child = parent;
          parent = tree.segments()[parent].parent_id;
        }
      }
    }

    std::vector<int> new_index(tree.segments().size());
    new_index[0] = 0;

    auto &new_tree = new_forest.trees[t];
    new_tree.segments().clear();
    new_tree.segments().push_back(tree.segments()[0]);
    // now going from root to tips, reindexing when below the
    // specified diameter
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      if (max_diameter[i] > 0.01 * diameter_value)
      {
        new_index[i] = static_cast<int>(new_tree.segments().size());
        new_tree.segments().push_back(tree.segments()[i]);
        new_tree.segments().back().parent_id = new_index[tree.segments()[i].parent_id];
      }
      else
      {
        new_index[i] = new_index[tree.segments()[i].parent_id];
      }
    }
    if (new_tree.segments().size() == 1)  // remove this tree
    {
      new_forest.trees[t] = new_forest.trees.back();
      new_forest.trees.pop_back();
      forest.trees[t] = forest.trees.back();
      forest.trees.pop_back();
      t--;
    }
  }
}

/// remove the specifiied length from the end of all branches
void pruneLength(ray::ForestStructure &forest, double length_value, ray::ForestStructure &new_forest)
{
  new_forest = forest;

  for (int t = 0; t < static_cast<int>(forest.trees.size()); t++)
  {
    auto &tree = forest.trees[t];
    // temporarily get the children list, it helps
    std::vector<std::vector<int>> children(tree.segments().size());
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      children[tree.segments()[i].parent_id].push_back(static_cast<int>(i));
    }
    // find the minimum length from leaf for every branch segment
    std::vector<double> min_length_from_leaf(tree.segments().size(), std::numeric_limits<double>::max());
    for (size_t i = 0; i < tree.segments().size(); i++)
    {
      if (children[i].size() == 0)  // a leaf, so ...
      {
        int parent = tree.segments()[i].parent_id;
        int child = static_cast<int>(i);

        min_length_from_leaf[i] = 0.0;
        while (parent != -1)  // iterate through the parents
        {
          double distance = (tree.segments()[parent].tip - tree.segments()[child].tip).norm();
          double new_dist = min_length_from_leaf[child] + distance;
          if (new_dist < min_length_from_leaf[parent])
          {
            min_length_from_leaf[parent] = new_dist;
          }
          else
          {
            break;
          }
          child = parent;
          parent = tree.segments()[parent].parent_id;
        }
      }
    }

    std::vector<int> new_index(tree.segments().size());
    new_index[0] = 0;

    auto &new_tree = new_forest.trees[t];
    new_tree.segments().clear();
    new_tree.segments().push_back(tree.segments()[0]);
    // now iterate from root to leaf, filling new_tree only up to the
    // minimum distance from end (length_value)
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      if (min_length_from_leaf[i] > length_value)
      {
        new_index[i] = static_cast<int>(new_tree.segments().size());
        new_tree.segments().push_back(tree.segments()[i]);
        new_tree.segments().back().parent_id = new_index[tree.segments()[i].parent_id];
      }
      else
      {
        int par = tree.segments()[i].parent_id;
        if (par != -1 && min_length_from_leaf[par] > length_value)
        {
          double blend = (min_length_from_leaf[par] - length_value) /
                         std::max(std::numeric_limits<double>::min(), min_length_from_leaf[par] - min_length_from_leaf[i]);
          blend = std::max(std::numeric_limits<double>::min(), std::min(blend, 1.0));
          ray::TreeStructure::Segment seg = tree.segments()[i];
          seg.tip = tree.segments()[par].tip + (seg.tip - tree.segments()[par].tip) * blend;

          new_index[i] = static_cast<int>(new_tree.segments().size());
          new_tree.segments().push_back(seg);
          new_tree.segments().back().parent_id = new_index[seg.parent_id];
        }
        else
        {
          new_index[i] = new_index[tree.segments()[i].parent_id];
        }
      }
    }
    if (new_tree.segments().size() == 1)  // remove this tree
    {
      new_forest.trees[t] = new_forest.trees.back();
      new_forest.trees.pop_back();
      forest.trees[t] = forest.trees.back();
      forest.trees.pop_back();
      t--;
    }
  }
}

}  // namespace tree
