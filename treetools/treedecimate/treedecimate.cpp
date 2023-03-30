// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <raylib/raycloud.h>
#include <raylib/rayforeststructure.h>
#include <raylib/rayparse.h>
#include <raylib/rayrenderer.h>
#include <raylib/raytreegen.h>
#include <cstdlib>
#include <iostream>
#include "raylib/raytreegen.h"
#include "treelib/treeutils.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Decimate the segments in the tree file, maintaining the topology and branch location geometry" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treedecimate forest.txt 2 segments - reduce to only every 2 segments, so roughly half the complexity" << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method reduces the number of segments in a tree file, while maintaining the topology of each tree.
/// The highest decimation therefore reduces to one cylindrical segment between two adjacent junctions
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  ray::TextArgument segments_text("segments");
  ray::IntArgument decimation;
  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file, &decimation, &segments_text });
  if (!parsed)
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
    std::cout << "decimate only works on tree structures, not trunks-only files" << std::endl;
    usage();
  }
  ray::ForestStructure new_forest = forest;

  // now, for each segment, we find the neighbours in range
  for (size_t t = 0; t < forest.trees.size(); t++)
  {
    auto &tree = forest.trees[t];
    // temporarily get the children list, it helps
    std::vector<std::vector<int>> children(tree.segments().size());
    for (size_t i = 1; i < tree.segments().size(); i++) 
    {
      children[tree.segments()[i].parent_id].push_back(static_cast<int>(i));
    }
    std::vector<int> new_index(tree.segments().size());
    std::vector<int> counts(tree.segments().size());  // count up from each branch point
    new_index[0] = 0;
    counts[0] = 0;

    auto &new_tree = new_forest.trees[t];
    new_tree.segments().clear();
    new_tree.segments().push_back(tree.segments()[0]);
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      counts[i] = counts[tree.segments()[i].parent_id] + 1;
      if (counts[i] == decimation.value() || children[i].size() > 1 || children[i].size() == 0)
      {
        new_index[i] = static_cast<int>(new_tree.segments().size());
        new_tree.segments().push_back(tree.segments()[i]);
        new_tree.segments().back().parent_id = new_index[tree.segments()[i].parent_id];
        counts[i] = 0;
      }
      else
      {
        new_index[i] = new_index[tree.segments()[i].parent_id];
      }
    }
  }
  new_forest.save(forest_file.nameStub() + "_decimated.txt");
  return 0;
}
