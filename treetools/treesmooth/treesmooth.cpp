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
  std::cout << "Smooth the segments in the tree file."
            << std::endl;
  std::cout << "To smooth further, run multiple times." << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treesmooth forest.txt" << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method smoothes the locations of the cylinders in the tree file, so that trunks and
/// thicker branches are proportionally straighter than the small branches.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  if (!ray::parseCommandLine(argc, argv, { &forest_file }))
  {
    usage();
  }

  double power = 0.0; // 1 will weight in proportion to radius, 2 in proportion to square radius.

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }
  if (forest.trees.size() != 0 && forest.trees[0].segments().size() == 0)
  {
    std::cout << "smooth only works on tree structures, not trunks-only files" << std::endl;
    usage();
  }

  // for each tree
  for (size_t t = 0; t < forest.trees.size(); t++)
  {
    auto &tree = forest.trees[t];
    // temporarily get the children list, it helps
    std::vector<std::vector<int>> children(tree.segments().size());
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      children[tree.segments()[i].parent_id].push_back(static_cast<int>(i));
    }
    const double full_w = std::pow(tree.segments()[0].radius, power);

    // smooth multiple times, in order to pass the information up/down the trees
    const int num_iterations = 2;
    for (int iteration = 0; iteration < num_iterations; iteration++)
    {
      // store old tips so smooth is not segment order dependent
      std::vector<Eigen::Vector3d> old_tips(tree.segments().size());
      for (size_t i = 0; i < tree.segments().size(); i++)
      {
        old_tips[i] = tree.segments()[i].tip;
      }

      Eigen::Vector3d root_shift(0, 0, 0);
      double root_weight = 0.0;
      for (size_t i = 1; i < tree.segments().size(); i++)
      {
        auto &segment = tree.segments()[i];
        const Eigen::Vector3d segment_tip = old_tips[i];
        const Eigen::Vector3d parent_tip = old_tips[segment.parent_id];
        const size_t num_kids = children[i].size();
        Eigen::Vector3d child_tip(0, 0, 0);
        // now smooth differently depending on the number of child branches
        if (num_kids == 0)  // end of branch. Usually thin so do nothing
        {
          continue;
        }
        else if (num_kids == 1)  // usual case
        {
          child_tip = old_tips[children[i][0]];
        }
        else  // a bifurcation point, so get a centroidal child tip
        {
          double weight = 0.0;
          for (auto &child : children[i])
          {
            const double rad_sqr = tree.segments()[child].radius * tree.segments()[child].radius;
            child_tip += old_tips[child] * rad_sqr;
            weight += rad_sqr;
          }
          if (weight > 0.0)  // should be, but just in case
          {
            child_tip /= weight;
          }
        }
        // now average the child tip
        const Eigen::Vector3d dir = (child_tip - parent_tip).normalized();
        const Eigen::Vector3d straight_tip = parent_tip + dir * (segment_tip - parent_tip).dot(dir);
        const double w = std::pow(segment.radius, power);
        const double blend = 0.5 * w / full_w;
        const Eigen::Vector3d new_tip = segment_tip * (1.0 - blend) + straight_tip * blend;
        if (segment.parent_id == -1)
        {
          // 50% otherwise it is disproportionately pliant
          root_shift += (segment.tip - new_tip) * 0.5 * w;
          root_weight += w;
        }
        segment.tip = new_tip;
      }
      if (root_weight > 0.0)
      {
        tree.segments()[0].tip += root_shift / root_weight;
      }
    }
  }
  forest.save(forest_file.nameStub() + "_smoothed.txt");
  return 0;
}
