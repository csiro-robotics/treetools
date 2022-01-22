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
  std::cout << "Set the per-segment tree foliage density from a ray cloud as the greyscale tree colour" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treefoliage forest.txt forest.ply 0.2 - set foliage density for given radius around segment" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, cloud_file;
  ray::DoubleArgument max_distance;
  bool parsed = ray::parseCommandLine(argc, argv, {&forest_file, &cloud_file, &max_distance});
  if (!parsed)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }  

  ray::Cloud::Info info;
  ray::Cloud::getInfo(cloud_file.name(), info);
  double voxel_width = 0.5*max_distance.value();
  Eigen::Vector3i dims = ((info.ends_bound.max_bound_ - info.ends_bound.min_bound_)/voxel_width).cast<int>() + Eigen::Vector3i(1,1,1);
  ray::DensityGrid grid(info.ends_bound, voxel_width, dims);
  grid.calculateDensities(cloud_file.name());
  #if DENSITY_MIN_RAYS > 0
  grid.addNeighbourPriors();
  #endif

  // now, for each segment, we find the neighbours in range
  for (auto &tree: forest.trees)
  {
    tree.attributes().push_back("red");
    tree.attributes().push_back("green");
    tree.attributes().push_back("blue");    
    for (size_t s = 1; s<tree.segments().size(); s++)
    {
      auto &segment = tree.segments()[s];
      Eigen::Vector3d p1 = segment.tip;
      Eigen::Vector3d p2 = tree.segments()[segment.parent_id].tip;
      double r = segment.radius + max_distance.value();
      Eigen::Vector3d minb = ray::minVector(p1, p2) - Eigen::Vector3d(r,r,r);
      Eigen::Vector3d maxb = ray::maxVector(p1, p2) + Eigen::Vector3d(r,r,r);
      Eigen::Vector3i mini = ((minb - info.ends_bound.min_bound_) / voxel_width).cast<int>();
      Eigen::Vector3i maxi = ((maxb - info.ends_bound.min_bound_) / voxel_width).cast<int>();
      double density = 0.0;
      double num_cells = 0;
      for (int i = std::max(0, mini[0]); i<=std::min(maxi[0], dims[0]-1); i++)
      {
        for (int j = std::max(0, mini[1]); j<=std::min(maxi[1], dims[1]-1); j++)
        {
          for (int k = std::max(0, mini[2]); k<=std::min(maxi[2], dims[2]-1); k++)
          {
            Eigen::Vector3d pos = Eigen::Vector3d(i+0.5,j+0.5,k+0.5) * voxel_width + info.ends_bound.min_bound_;
            double d = std::max(0.0, std::min((pos - p1).dot(p2-p1) / (p2-p1).squaredNorm(), 1.0));
            Eigen::Vector3d nearest = p1 + (p2-p1)*d;
            double distance = (nearest - pos).norm();
            if (distance > r)
              continue;
            density += grid.voxels()[grid.getIndex(Eigen::Vector3i(i,j,k))].density();
            num_cells++;
          }
        }
      }
      // TODO: what to do here? divide density by volume of branch? 
      if (num_cells > 0.0)
        density /= num_cells;
      Eigen::Vector3d col(density, density, density);  
      segment.attributes.push_back(col[0]);
      segment.attributes.push_back(col[1]);
      segment.attributes.push_back(col[2]);
    }
  }
}


