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
  std::cout << "Smooth the segments in the tree file, fully at the trunk and proportionally less with radius." << std::endl;
  std::cout << "To smooth further, run multiple times." << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treesmooth forest.txt" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  if (!ray::parseCommandLine(argc, argv, {&forest_file}))
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
    std::cout << "smooth only works on tree structures, not trunks-only files" << std::endl;
    usage();
  }

  for (size_t t = 0; t<forest.trees.size(); t++)
  {
    auto &tree = forest.trees[t];
    // temporarily get the children list, it helps
    std::vector< std::vector<int> > children(tree.segments().size());
    for (size_t i = 1; i<tree.segments().size(); i++)
    {
      children[tree.segments()[i].parent_id].push_back((int)i);
    }
    double full_rad_sqr = tree.segments()[0].radius * tree.segments()[0].radius;

    const int num_iterations = 4;
    for (int iteration = 0; iteration<num_iterations; iteration++)
    {
      std::vector<Eigen::Vector3d> old_tips(tree.segments().size());
      for (size_t i = 0; i<tree.segments().size(); i++)
      {
        old_tips[i] = tree.segments()[i].tip;
      }

      Eigen::Vector3d root_shift(0,0,0);
      double root_weight = 0.0;
      for (size_t i = 1; i<tree.segments().size(); i++)
      {
        auto &segment = tree.segments()[i];
        Eigen::Vector3d segment_tip = old_tips[i];
        Eigen::Vector3d parent_tip = old_tips[segment.parent_id];
        size_t num_kids = children[i].size();
        Eigen::Vector3d child_tip(0,0,0);
        if (num_kids == 0) // end of branch. Usually thin so do nothing
        {
          continue;
        }
        else if (num_kids == 1) // usual case
        {
          child_tip = old_tips[children[i][0]];
        }
        else // a bifurcation point, so get a centroidal child tip
        {
          double weight = 0.0;
          for (auto &child: children[i])
          {
            double rad = tree.segments()[child].radius;
            double rad_sqr = rad * rad;
            child_tip += old_tips[child] * rad_sqr;
            weight += rad_sqr;
          }
          if (weight > 0.0) // should be, but just in case
          {
            child_tip /= weight;
          }
        }
        Eigen::Vector3d dir = (child_tip - parent_tip).normalized();
        Eigen::Vector3d straight_tip = parent_tip + dir*(segment_tip - parent_tip).dot(dir);
        double rad_sqr = segment.radius * segment.radius;
        double blend = 0.5 * rad_sqr / full_rad_sqr;
        Eigen::Vector3d new_tip = segment_tip * (1.0 - blend) + straight_tip * blend;
        if (segment.parent_id == -1)
        {
          root_shift += (segment.tip - new_tip) * 0.5 * rad_sqr; // 50% otherwise it is disproportionately pliant
          root_weight += rad_sqr;
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
}

