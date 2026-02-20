// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <raylib/raycloud.h>
#include <raylib/raycloudwriter.h>
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
  std::cout << "Set the per-segment tree foliage density from a ray cloud as the greyscale tree colour" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treefoliage forest.txt forest.ply 0.2 - set foliage density for given radius around segment"
            << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method sets a foliage_density (and foliage_sparsity) attribute per-segment into the tree file,
/// by estimating the one-sided leaf area density in the specified accompanying ray cloud.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, cloud_file;
  ray::DoubleArgument max_distance;
  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file, &cloud_file, &max_distance });
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
  const double voxel_width = 0.5 * max_distance.value();
  const Eigen::Vector3i dims =
    ((info.ends_bound.max_bound_ - info.ends_bound.min_bound_) / voxel_width).cast<int>() + Eigen::Vector3i(1, 1, 1);
  ray::DensityGrid grid(info.ends_bound, voxel_width, dims);
  grid.calculateDensities(cloud_file.name());
  double mxd = 0.0;
  for (auto &vox : grid.voxels())
  {
    mxd = std::max(mxd, vox.density());
  }
  grid.addNeighbourPriors();
  double mxd2 = 0.0;
  for (auto &vox : grid.voxels())
  {
    mxd2 = std::max(mxd2, vox.density());
  }
  std::cout << "maximum density before: " << mxd << ", after: " << mxd2 << std::endl;
  // now, for each segment, we find the neighbours in range
  double max_density = 0.0;
  for (auto &tree : forest.trees)
  {
    tree.attributeNames().push_back("foliage_density");
    tree.attributeNames().push_back("foliage_sparsity");
    double tree_density = 0.0;
    double tree_weight = 0.0;
    for (size_t s = 1; s < tree.segments().size(); s++)
    {
      auto &segment = tree.segments()[s];
      const Eigen::Vector3d p1 = segment.tip;
      const Eigen::Vector3d p2 = tree.segments()[segment.parent_id].tip;
      const double r = segment.radius + max_distance.value();
      const Eigen::Vector3d minb = ray::minVector(p1, p2) - Eigen::Vector3d(r, r, r);
      const Eigen::Vector3d maxb = ray::maxVector(p1, p2) + Eigen::Vector3d(r, r, r);
      const Eigen::Vector3i mini = ((minb - info.ends_bound.min_bound_) / voxel_width).cast<int>();
      const Eigen::Vector3i maxi = ((maxb - info.ends_bound.min_bound_) / voxel_width).cast<int>();
      double total_density = 0.0;
      double num_cells = 0;
      for (int i = std::max(0, mini[0]); i <= std::min(maxi[0], dims[0] - 1); i++)
      {
        for (int j = std::max(0, mini[1]); j <= std::min(maxi[1], dims[1] - 1); j++)
        {
          for (int k = std::max(0, mini[2]); k <= std::min(maxi[2], dims[2] - 1); k++)
          {
            const Eigen::Vector3d pos =
              Eigen::Vector3d(i + 0.5, j + 0.5, k + 0.5) * voxel_width + info.ends_bound.min_bound_;
            const double d = std::max(0.0, std::min((pos - p1).dot(p2 - p1) / (p2 - p1).squaredNorm(), 1.0));
            const Eigen::Vector3d nearest = p1 + (p2 - p1) * d;
            const double distance = (nearest - pos).norm();
            if (distance > r)
            {
              continue;
            }
            const double density = grid.voxels()[grid.getIndex(Eigen::Vector3i(i, j, k))].density();
            max_density = std::max(max_density, density);
            total_density += density;
            num_cells++;
          }
        }
      }
      tree_density += total_density;
      tree_weight += num_cells;
      if (num_cells > 0.0)
      {
        total_density /= num_cells;
      }
      segment.attributes.push_back(total_density);
    }
    tree.segments()[0].attributes.push_back(0);
    // next we average the per-segment foliage densities over each whole subtree:
    std::vector<std::vector<int>> children(tree.segments().size());
    for (size_t i = 0; i < tree.segments().size(); i++)
    {
      if (tree.segments()[i].parent_id != -1)
      {
        children[tree.segments()[i].parent_id].push_back(static_cast<int>(i));
      }
    }
    std::vector<double> densities(tree.segments().size());
    for (size_t i = 0; i < tree.segments().size(); i++)
    {
      densities[i] = tree.segments()[i].attributes.back();
      double num = i == 0 ? 0.0 : 1.0;
      std::vector<int> segments = children[i];
      for (size_t b = 0; b < segments.size(); b++)
      {
        int seg_id = segments[b];
        densities[i] += tree.segments()[seg_id].attributes.back();
        num++;
        segments.reserve(segments.size() + children[seg_id].size());
        segments.insert(segments.end(), std::begin(children[seg_id]), std::end(children[seg_id]));
      }
      if (num > 0.0)
      {
        densities[i] /= num;
      }
    }
    for (size_t i = 0; i < tree.segments().size(); i++)
    {
      tree.segments()[i].attributes.back() = densities[i];
      tree.segments()[i].attributes.push_back(densities[i] == 0.0 ? 0.0 : 1.0 / densities[i]);
    }
  }

  forest.save(forest_file.nameStub() + "_foliage.txt");

  ray::CloudWriter writer;
  if (!writer.begin(cloud_file.nameStub() + "_densities.ply"))
  {
    usage();
  }
  auto colour = [&](std::vector<Eigen::Vector3d> &starts, std::vector<Eigen::Vector3d> &ends,
                    std::vector<double> &times, std::vector<ray::RGBA> &colours) {
    for (size_t i = 0; i < ends.size(); i++)
    {
      if (colours[i].alpha == 0)
      {
        continue;
      }
      const Eigen::Vector3d pos = ends[i];
      const double density = grid.voxels()[grid.getIndexFromPos(pos)].density();
      const uint8_t shade = (uint8_t)std::max(0.0, std::min(3.0 * 255.0 * density / max_density, 255.0));
      colours[i].red = colours[i].green = colours[i].blue = shade;
    }
    writer.writeChunk(starts, ends, times, colours);
  };

  if (!ray::Cloud::read(cloud_file.name(), colour))
  {
    usage();
  }
  writer.end();
  return 0;
}
