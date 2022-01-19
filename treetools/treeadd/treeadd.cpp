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
#include <cstdlib>
#include <iostream>

void usage(int exit_code = 1)
{
  std::cout << "Add information to a tree file" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treeadd forest.txt foliage_density forest_cloud.ply - adds the foliage density from the cloud into the tree file" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, cloud_file;
  ray::TextArgument density("foliage_density");
  bool parsed = ray::parseCommandLine(argc, argv, {&forest_file, &density, &cloud_file});
  if (!parsed)
  {
    usage();
  }

  ray::ForestGen forest;
  ray::ForestParams params;
  if (!forest.makeFromFile(forest_file.name(), params))
  {
    usage();
  }  
  std::vector<ray::TreeGen> &trees = forest.trees();

  ray::Cloud cloud;
  cloud.load(cloud_file.name());

  // there are two cases:
  // 1. trunks only: I need to know the bounds of the trees... if each tree is a separate colour this can be done
  // 2. all branches: we specify a maximum distance from the branch, and otherwise use weighted Voronoi cells per branch...
  //    we may want to save out the numerator and denominator, so users can do a better average, e.g. up the branch. 

  // in both cases we need to voxelise the cloud and calculate the density using ray walking
}


