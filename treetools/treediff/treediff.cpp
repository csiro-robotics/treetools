// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <nabo/nabo.h>
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
  std::cout << "                              --surface_area - estimates error between surfaces- Root Mean Square per surface patch" << std::endl;
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

double getMinDistanceSqr(const Eigen::Vector3d &start1, const Eigen::Vector3d &end1, const Eigen::Vector3d &start2, const Eigen::Vector3d &end2)
{
  Eigen::Vector3d v1 = end1 - start1;
  Eigen::Vector3d v2 = end2 - start2;
  Eigen::Vector3d between = v1.cross(v2);
  Eigen::Vector3d norm2 = v2.cross(between);
  double den1 = v1.dot(norm2);
  double t1 = std::max(0.0, std::min((start2 - start1).dot(norm2) / (den1 ? den1 : 1.0), 1.0)); 
  Eigen::Vector3d norm1 = v1.cross(between);
  double den2 = v2.dot(norm1);
  double t2 = std::max(0.0, std::min((start1 - start2).dot(norm1) / (den2 ? den2 : 1.0), 1.0)); 
  return ((start1 + t1*v1) - (start2 + t2*v2)).squaredNorm();
}

double sqr(double x) { return x*x; }

void printSurfaceRMSE(const std::vector<ray::TreeStructure> &trees1, const std::vector<ray::TreeStructure> &trees2, const std::vector<int> &trunk_matches)
{
  std::vector<Eigen::Vector3d> points1, points2;
  std::vector<double> radii1, radii2;
  std::vector<Eigen::Vector3d> parents1, parents2;
  const double rad_scale = 4.0; // how much to match the radius when finding nearest neighbours
  for (auto &tree: trees1)
  {
    for (int i = 1; i<(int)tree.segments().size(); i++)
    {
      auto &seg = tree.segments()[i];
      points1.push_back(seg.tip); 
      radii1.push_back(seg.radius);
      parents1.push_back(tree.segments()[seg.parent_id].tip);
    }
  }
  for (int t = 0; t<(int)trees1.size(); t++)
  {
    auto &tree = trees2[trunk_matches[t]];
    for (int i = 1; i<(int)tree.segments().size(); i++)
    {
      auto &seg = tree.segments()[i];
      points2.push_back(seg.tip); 
      radii2.push_back(seg.radius);
      parents2.push_back(tree.segments()[seg.parent_id].tip);
    }
  }
  int search_size = 3;
  size_t q_size = points1.size();
  size_t p_size = points2.size();
  Nabo::NNSearchD *nns;
  Eigen::MatrixXd points_q(4, q_size);
  for (size_t i = 0; i < q_size; i++)
  {
    points_q.col(i) << points1[i], radii1[i]*rad_scale;
  }
  Eigen::MatrixXd points_p(4, p_size);
  for (size_t i = 0; i < p_size; i++)
  {
    points_p.col(i) << points2[i], radii2[i]*rad_scale;
  }
  nns = Nabo::NNSearchD::createKDTreeLinearHeap(points_p, 4);

  // Run the search
  Eigen::MatrixXi indices;
  Eigen::MatrixXd dists2;
  indices.resize(search_size, q_size);
  dists2.resize(search_size, q_size);
  nns->knn(points_q, indices, dists2, search_size, ray::kNearestNeighbourEpsilon, 0, 1.0);
  delete nns;

  double total_squared_error = 0.0;
  double total_weight = 0.0;
  for (int i = 0; i < (int)q_size; i++)
  {
    double min_scaled_dist_sqr = 1e10; // we use a scaled radius for finding the best match
    double min_dist_sqr = 0.0;        // but a normal radius for finding the actual square distance
    double min_weight = 0.0;
    for (int k = 0; k < search_size && indices(k, i) != Nabo::NNSearchD::InvalidIndex; k++)
    {
      int j = indices(k, i);
      double distance_sqr = getMinDistanceSqr(points1[i], parents1[i], points2[j], parents2[j]);
      double rad_sqr = sqr(radii1[i] - radii2[j]);
      double scaled_rad_sqr = rad_sqr * rad_scale*rad_scale;
      const double pi = 3.1415926;
      double surface_area1 = pi*sqr(radii1[i])*(points1[i]-parents1[i]).norm();
      double surface_area2 = pi*sqr(radii2[j])*(points2[j]-parents2[j]).norm();
      double weight = surface_area1 + surface_area2;
      double dist_sqr = distance_sqr + rad_sqr;
      double scaled_dist_sqr = distance_sqr + scaled_rad_sqr;
      if (scaled_dist_sqr < min_scaled_dist_sqr)
      {
        min_scaled_dist_sqr = scaled_dist_sqr;
        min_dist_sqr = dist_sqr;
        min_weight = weight;
      }
    }
    total_weight += min_weight;
    total_squared_error += min_dist_sqr*min_weight;
  }
  double root_mean_sqr = std::sqrt(total_squared_error / total_weight);
  std::cout << " for overlapping trunks: approximate surface RMSE: " << root_mean_sqr << " m" << std::endl;
}

/// This method outputs the difference between two tree files. In particular, what percentage of
/// trees overlap each other, and statistics of the similarity of the set of overlapping trees.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file1, forest_file2;
  ray::OptionalFlagArgument include_growth("include_growth", 'i');
  ray::OptionalFlagArgument surface_area("surface_area", 's');
  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file1, &forest_file2 }, { &include_growth, &surface_area });
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
    double min_overlap = std::numeric_limits<double>::max();
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
  mean_overlap /= static_cast<double>(num_matches);
  std::cout << std::endl;
  std::cout << 100.0 * static_cast<double>(num_matches) / static_cast<double>(trees1.size() + trees2.size() - num_matches) << "% overlap of trees ";
  std::cout << "(#trees 1: " << trees1.size() << ", #trees 2: " << trees2.size() << ", #overlapping: " << num_matches << ")" << std::endl;
  std::cout << std::endl;
  std::cout << "of the matching trees: " << std::endl;
  std::cout << " mean trunk overlap: " << 100.0*(1.0 - mean_overlap) << "%, mean growth in trunk radius: " << 100.0 * (mean_radius2 / mean_radius1 - 1.0) << "%" << std::endl;

  // if we only have trunk information then this is as far as we get
  if (forest1.trees[0].segments().size() == 1 || forest2.trees[0].segments().size() == 1)
  {
    return 0;
  }

  if (surface_area.isSet())
  {
    printSurfaceRMSE(trees1, trees2, trunk_matches);
  }


  double mean_growth = 0;
  double max_growth = 0.0;
  double min_growth = std::numeric_limits<double>::max();
  double total_overlap = 0;
  double total_overlap_weight = 0;
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
    double max_overlap_weight = 0.0;
    if (include_growth.isSet())
    {
      double scale_range = 0.5;  // actually the half-range
      const double divisions = 5.0;
      while (scale_range > 0.02)
      {
        max_overlap_percent = 0.0;
        max_overlap_weight = 0.0;
        double max_overlap_scale = 0;
        for (double rad_scale = scale_mid - scale_range; rad_scale <= scale_mid + scale_range;
             rad_scale += scale_range / divisions)
        {
          const double overlap = treeOverlapVolume(tree1, tree2, rad_scale);
          const double overlap_weight = (rad_scale * rad_scale * tree1_volume + tree2_volume - overlap);
          const double overlap_percent = overlap / overlap_weight;
          if (overlap_percent > max_overlap_percent)
          {
            max_overlap = overlap;
            max_overlap_percent = overlap_percent;
            max_overlap_scale = rad_scale;
            max_overlap_weight = overlap_weight;
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
      max_overlap_weight = (tree1_volume + tree2_volume - max_overlap);
      max_overlap_percent = max_overlap / max_overlap_weight;
    }

    total_overlap += max_overlap;
    total_overlap_weight += max_overlap_weight;

    // now we have a scale match, we need to look for change in volume:
    double removed_volume = std::max(0.0, scale_mid * scale_mid * tree1_volume - max_overlap);
    double added_volume = std::max(0.0, tree2_volume - max_overlap);
    total_volume += tree2_volume;
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

  mean_growth /= static_cast<double>(num_matches);
  total_volume /= static_cast<double>(num_matches);
  mean_added_volume /= static_cast<double>(num_matches);
  mean_removed_volume /= static_cast<double>(num_matches);
  std::cout << " tree overlap (Intersection Over Union): " << 100.0 * (total_overlap / total_overlap_weight) << "%" << std::endl;
  if (include_growth.isSet())
  {
    std::cout << " mean radius growth: " << 100.0 * (mean_growth - 1.0)
              << "%, min growth: " << 100.0 * (min_growth - 1.0) << "%, max growth: " << 100.0 * (max_growth - 1.0)
              << "%" << std::endl;
    std::cout << " after scaling each tree to match new version:" << std::endl;
  }

  std::cout << " added volume: " << 100.0 * (mean_added_volume / total_volume) << "%" << std::endl;
  std::cout << " removed volume: " << 100.0 * (mean_removed_volume / total_volume) << "%" << std::endl;

  std::cout << " maximum added volume " << max_added_volume << " m^3 for tree at "
            << trees1[max_add_i].root().transpose() << " (ids " << max_add_i << ", " << trunk_matches[max_add_i] << ")"<< std::endl;
  std::cout << " maximum removed volume " << max_removed_volume << " m^3 for tree at "
            << trees1[max_removal_i].root().transpose() << " (ids " << max_removal_i << ", " << trunk_matches[max_removal_i] << ")" << std::endl;
  return 0;
}
