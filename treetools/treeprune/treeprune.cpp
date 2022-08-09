// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <raylib/raycloud.h>
#include <raylib/rayparse.h>
#include <raylib/rayrenderer.h>
#include "treelib/treepruner.h"
#include "treelib/treeutils.h"

#include <cstdlib>
#include <iostream>

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Prune branches less than a diameter, or by a chosen length" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treeprune forest.txt 2 cm       - cut off branches less than 2 cm wide" << std::endl;
  std::cout << "                     0.5 m long - cut off branches less than 0.5 m long" << std::endl;
  // clang-format on
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  ray::DoubleArgument diameter(0.0001, 100.0), length(0.001, 1000.0);
  ray::TextArgument cm("cm"), m("m"), long_text("long");
  ray::KeyValueChoice choice({ "diameter", "length" }, { &diameter, &length });

  bool prune_diameter = ray::parseCommandLine(argc, argv, { &forest_file, &diameter, &cm });
  bool prune_length = ray::parseCommandLine(argc, argv, { &forest_file, &length, &m, &long_text });
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
  ray::ForestStructure new_forest;
  if (prune_diameter)
    tree::pruneDiameter(forest, diameter.value(), new_forest);
  else if (prune_length)
    tree::pruneLength(forest, length.value(), new_forest);


  if (new_forest.trees.empty())
  {
    std::cout << "Warning: no trees left after pruning. No file saved." << std::endl;
  }
  else
  {
    new_forest.save(forest_file.nameStub() + "_pruned.txt");
  }
}
