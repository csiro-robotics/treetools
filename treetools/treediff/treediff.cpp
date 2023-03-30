// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <raylib/rayforeststructure.h>
#include <raylib/rayparse.h>
#include <raylib/raytreegen.h>
#include <cstdlib>
#include <iostream>
#include "treelib/treeutils.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Difference information on two tree files" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treediff forest1.txt forest2.txt - difference information from forest1 to forest2" << std::endl;
  std::cout << "                            --include_growth - estimates radius growth of tree (slower)" << std::endl;
  // clang-format on
  exit(exit_code);
}

/// @brief returns an approximation of the overlapping volume between two tree structures @c tree1 and @c tree2
/// @param tree1_rad_scale the dilation of tree1
/// @return overlapping volume
double treeOverlapVolume(const ray::TreeStructure &tree1, const ray::TreeStructure &tree2, double tree1_rad_scale)
{
  double volume = 0.0;
  const double eps = 1e-7;
  for (size_t i = 1; i < tree1.segments().size(); i++)
  {
    auto &branch = tree1.segments()[i];
    const Eigen::Vector3d base = tree1.segments()[branch.parent_id].tip;
    const tree::Cylinder cyl1(branch.tip, base, tree1_rad_scale * branch.radius);
    for (size_t j = 1; j < tree2.segments().size(); j++)
    {
      auto &other = tree2.segments()[j];
      const Eigen::Vector3d base2 = tree2.segments()[other.parent_id].tip;
      const tree::Cylinder cyl2(other.tip, base2, other.radius);
      if ((cyl2.v2 - cyl2.v1).squaredNorm() < eps)
        continue;
      volume += tree::approximateIntersectionVolume(cyl1, cyl2);
    }
  }
  return volume;
}

/// This method outputs the difference between two tree files. In particular, what percentage of
/// trees overlap each other, and statistics of the similarity of the set of overlapping trees.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file1, forest_file2;
  ray::OptionalFlagArgument include_growth("include_growth", 'i');
  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file1, &forest_file2 }, { &include_growth });
  if (!parsed)
  {
    usage();
  }

  ray::ForestStructure forest1, forest2;
  if (!forest1.load(forest_file1.name()) || !forest2.load(forest_file2.name()))
  {
    usage();
  }
  std::vector<ray::TreeStructure> &trees1 = forest1.trees;
  std::vector<ray::TreeStructure> &trees2 = forest2.trees;

  // first, find the amount of overlap in the tree trunks based on radius. This is the same code for trunks only and
  // full tree text files
  std::vector<int> trunk_matches(trees1.size(), -1);
  int num_matches = 0;
  double mean_overlap = 0;
  double mean_radius1 = 0, mean_radius2 = 0.0;
  for (size_t i = 0; i < trees1.size(); i++)
  {
    auto &tree1 = trees1[i];
    double min_overlap = 1e10;
    double min_overlap_rad = 0;
    for (size_t j = 0; j < trees2.size(); j++)
    {
      if (std::find(trunk_matches.begin(), trunk_matches.end(), j) != trunk_matches.end())
      {
        continue;  // don't look at matches we have already made
      }
      auto &tree2 = trees2[j];
      double total_radius = tree1.segments()[0].radius + tree2.segments()[0].radius;
      Eigen::Vector3d dif = tree1.segments()[0].tip - tree2.segments()[0].tip;
      dif[2] = 0.0;
      const double overlap = dif.norm() / total_radius;
      if (overlap < min_overlap && overlap < 1.0)
      {
        min_overlap = overlap;
        trunk_matches[i] = static_cast<int>(j);
        min_overlap_rad = tree2.segments()[0].radius;
      }
    }
    if (trunk_matches[i] != -1)
    {
      num_matches++;
      mean_overlap += min_overlap;
      mean_radius1 += tree1.segments()[0].radius;
      mean_radius2 += min_overlap_rad;
    }
  }
  if (num_matches == 0)
  {
    std::cout << "no matches found between trees." << std::endl;
    return 0;
  }
  mean_overlap /= (double)num_matches;
  std::cout << 100.0 * (double)num_matches / (double)trees2.size() << "% of " << forest_file2.nameStub()
            << " trees overlap the " << forest_file1.nameStub() << " trees" << std::endl;
  std::cout << "#trees 1: " << trees1.size() << ", #trees 2: " << trees2.size() << ", #overlapping: " << num_matches
            << std::endl;
  std::cout << "mean trunk overlap: " << 1.0 - mean_overlap
            << ", mean growth in trunk radius: " << 100.0 * (mean_radius2 / mean_radius1 - 1.0) << "%" << std::endl;

  // if we only have trunk information then this is as far as we get
  if (forest1.trees[0].segments().size() == 1 || forest2.trees[0].segments().size() == 1)
  {
    return 0;
  }
  std::cout << std::endl;

  double mean_growth = 0;
  double max_growth = 0.0;
  double min_growth = 1e10;
  double mean_overlap_percent = 0;
  double total_volume = 0;
  double mean_added_volume = 0;
  double mean_removed_volume = 0;
  double max_removed_volume = 0;
  int max_removal_i = -1;
  double max_added_volume = 0;
  int max_add_i = -1;

  for (size_t i = 0; i < trees1.size(); i++)
  {
    if (trunk_matches[i] == -1)
    {
      continue;
    }
    auto &tree1 = trees1[i];
    auto &tree2 = trees2[trunk_matches[i]];
    const double tree1_volume = tree1.volume();
    const double tree2_volume = tree2.volume();

    // now we need to home in on the growth difference between the two versions of the trees.
    // here we assume that the point cloud aligning was perfect, so just adjust the scale for best match.

    double scale_mid = 1.0;
    double max_overlap = 0.0;
    double max_overlap_percent = 0.0;
    if (include_growth.isSet())
    {
      double scale_range = 0.5;  // actually the half-range
      const double divisions = 5.0;
      while (scale_range > 0.02)
      {
        max_overlap_percent = 0.0;
        double max_overlap_scale = 0;
        for (double rad_scale = scale_mid - scale_range; rad_scale <= scale_mid + scale_range;
             rad_scale += scale_range / divisions)
        {
          const double overlap = treeOverlapVolume(tree1, tree2, rad_scale);
          const double overlap_percent = overlap * 2.0 / (rad_scale * rad_scale * tree1_volume + tree2_volume);
          if (overlap_percent > max_overlap_percent)
          {
            max_overlap = overlap;
            max_overlap_percent = overlap_percent;
            max_overlap_scale = rad_scale;
          }
        }
        if (max_overlap_scale == 0.0)
        {
          std::cout << "error: trunks overlap but no overlap scale found. This shouldn't happen" << std::endl;
        }
        scale_mid = max_overlap_scale;
        scale_range /= divisions;
      }
      mean_growth += scale_mid;
      max_growth = std::max(max_growth, scale_mid);
      min_growth = std::min(min_growth, scale_mid);
    }
    else
    {
      max_overlap = treeOverlapVolume(tree1, tree2, scale_mid);
      max_overlap_percent = max_overlap * 2.0 / (tree1_volume + tree2_volume);
    }

    mean_overlap_percent += max_overlap_percent;

    // now we have a scale match, we need to look for change in volume:
    double removed_volume = std::max(0.0, scale_mid * scale_mid * tree1_volume - max_overlap);
    double added_volume = std::max(0.0, tree2_volume - max_overlap);
    total_volume += max_overlap;
    mean_added_volume += added_volume;
    mean_removed_volume += removed_volume;
    if (removed_volume > max_removed_volume)
    {
      max_removed_volume = removed_volume;
      max_removal_i = static_cast<int>(i);
    }
    if (added_volume > max_added_volume)
    {
      max_added_volume = added_volume;
      max_add_i = static_cast<int>(i);
    }
  }

  mean_growth /= (double)num_matches;
  total_volume /= (double)num_matches;
  mean_added_volume /= (double)num_matches;
  mean_removed_volume /= (double)num_matches;
  mean_overlap_percent /= (double)num_matches;
  std::cout << "mean tree percentage overlap: " << 100.0 * mean_overlap_percent << "%" << std::endl;
  if (include_growth.isSet())
  {
    std::cout << "mean radius growth: " << 100.0 * (mean_growth - 1.0)
              << "%, min growth: " << 100.0 * (min_growth - 1.0) << "%, max growth: " << 100.0 * (max_growth - 1.0)
              << "%" << std::endl;
    std::cout << "after scaling each tree to match new version:" << std::endl;
  }

  std::cout << "added volume: " << 100.0 * (mean_added_volume / total_volume) << "% which averages "
            << mean_added_volume << " m^3 per tree." << std::endl;
  std::cout << "maximum added volume " << max_added_volume << " m^3 for tree at "
            << trees1[max_add_i].root().transpose() << std::endl;

  std::cout << "removed volume: " << 100.0 * (mean_removed_volume / total_volume) << "% which averages "
            << mean_removed_volume << " m^3 per tree." << std::endl;
  std::cout << "maximum removed volume " << max_removed_volume << " m^3 for tree at "
            << trees1[max_removal_i].root().transpose() << std::endl;
  return 0;
}
