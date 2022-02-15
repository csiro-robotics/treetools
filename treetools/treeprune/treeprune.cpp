// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#include <raylib/rayforestgen.h>
#include <raylib/raytreegen.h>
#include <raylib/rayparse.h>
#include <raylib/raycloud.h>
#include <raylib/rayrenderer.h>
#include "raylib/raytreegen.h"
#include <cstdlib>
#include <iostream>

void usage(int exit_code = 1)
{
  std::cout << "Prune branches less than a diameter, or by a chosen length" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treeprune forest.txt 2 cm       - cut off branches less than 2 cm wide" << std::endl;
  std::cout << "                     0.5 m long - cut off branches less than 0.5 m long" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  ray::DoubleArgument diameter(0.0001, 100.0), length(0.001, 1000.0);
  ray::TextArgument cm("cm"), m("m"), long_text("long");
  ray::KeyValueChoice choice({"diameter", "length"}, 
                             {&diameter, &length});

  bool prune_diameter = ray::parseCommandLine(argc, argv, {&forest_file, &diameter, &cm});
  bool prune_length = ray::parseCommandLine(argc, argv, {&forest_file, &length, &m, &long_text});
  if (!prune_diameter && !prune_length)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }  
  if (forest.trees.size() != 0 && forest.trees[0].segments().size() == 0)
  {
    std::cout << "prune only works on tree structures, not trunks-only files" << std::endl;
    usage();
  }
  ray::ForestStructure new_forest = forest;

  for (int t = 0; t<(int)forest.trees.size(); t++)
  {
    auto &tree = forest.trees[t];
    // temporarily get the children list, it helps
    std::vector< std::vector<int> > children(tree.segments().size());
    std::vector<bool> enabled(tree.segments().size());
    std::vector<double> min_length_from_leaf(tree.segments().size(), 1e10);
    std::vector<double> max_diameter(tree.segments().size(), 0);
    std::vector<int> leaves;
    for (size_t i = 1; i<tree.segments().size(); i++)
      children[tree.segments()[i].parent_id].push_back((int)i);
    for (size_t i = 0; i<tree.segments().size(); i++)
    {
      if (children[i].size() == 0) // a leaf, so ...
      {
        int parent = tree.segments()[i].parent_id;
        int child = (int)i;
        max_diameter[child] = 2.0 * tree.segments()[child].radius;
        if (prune_diameter) // Here max_diameter ensures a branch diameter that only increases from the leaves
        {
          while (parent != -1)
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
        else  // distance from leaves
        {
          min_length_from_leaf[i] = 0.0;
          while (parent != -1)
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
    }

    std::vector<int> new_index(tree.segments().size());
    new_index[0] = 0;

    auto &new_tree = new_forest.trees[t];
    new_tree.segments().clear();
    new_tree.segments().push_back(tree.segments()[0]);
    for (size_t i = 1; i<tree.segments().size(); i++)
    {
      if ((prune_diameter && max_diameter[i] > 0.01*diameter.value()) || 
          (prune_length && min_length_from_leaf[i] > length.value())) 
      {
        new_index[i] = (int)new_tree.segments().size();
        new_tree.segments().push_back(tree.segments()[i]);
        new_tree.segments().back().parent_id = new_index[tree.segments()[i].parent_id];
      }
      else
      {
        new_index[i] = new_index[tree.segments()[i].parent_id];
      }
    }
    if (new_tree.segments().size()==1) // remove this tree
    {
      new_forest.trees[t] = new_forest.trees.back();
      new_forest.trees.pop_back();
      forest.trees[t] = forest.trees.back();
      forest.trees.pop_back();
      t--;
    }
  }
  if (new_forest.trees.empty())
  {
    std::cout << "Warning: no trees left after pruning. No file saved." << std::endl;
  }
  else
  {
    new_forest.save(forest_file.nameStub() + "_pruned.txt");
  }
}


