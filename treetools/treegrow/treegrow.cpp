// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <raylib/raycloud.h>
#include <raylib/rayparse.h>
#include <raylib/rayrenderer.h>
#include <cstdlib>
#include <iostream>
#include "raylib/raytreegen.h"
#include "treelib/treepruner.h"
#include "treelib/treeutils.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Placeholder method to grow or shrink the tree from the tips, using a linear model. No branches are added." << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treegrow forest.txt 1 years             - reduce the tree according to the rates, default values below." << std::endl;
  std::cout << "                    --length_rate 0.3   - expected branch length increase per year in m" << std::endl;
  std::cout << "                    --shed              - shed branches to maintain branch length power law" << std::endl;
  std::cout << "                    --prune_length 1    - length from tip that reconstructed trees are pruned to" << std::endl;
  // clang-format on
  exit(exit_code);
}

void addSubTree(std::vector<ray::TreeStructure::Segment> &segments, int root_id, 
  const Eigen::Vector3d &dir, const Eigen::Vector3d &side_dir, double new_branch_length,
  double k1, double k2, double angle1, double branch_angle, double prune_length)
{
  double bifurcate_distance = new_branch_length*(1.0 - k1);
  int par_id = segments[root_id].parent_id;
  if (new_branch_length < prune_length)
  {
    std::cout << "weird, this shouldn't happen" << std::endl;
    return;
  }
  if (new_branch_length*k2 < prune_length)
  {
    segments[root_id].tip = segments[par_id].tip + dir * (new_branch_length - prune_length);
    return;
  }
  segments[root_id].tip = segments[par_id].tip + dir * bifurcate_distance;
  
  // now: how do we end this escapade? We need to cut off at the prune length actually...

  ray::TreeStructure::Segment child1;
  child1.parent_id = root_id;
  child1.radius = segments[root_id].radius * k1;
  Eigen::Vector3d dir1 = dir * std::cos(angle1) + side_dir * std::sin(angle1);
  Eigen::Vector3d rand_dir1(ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5);
  Eigen::Vector3d side_dir1 = rand_dir1.cross(dir1).normalized();
  child1.tip = segments[root_id].tip + dir1 * k1;
  child1.attributes = segments[root_id].attributes;
  segments.push_back(child1);
  addSubTree(segments, (int)segments.size()-1, dir1, side_dir1, new_branch_length*k1,
    k1, k2, angle1, branch_angle, prune_length);

  ray::TreeStructure::Segment child2;
  child2.parent_id = root_id;
  child2.radius = segments[root_id].radius * k2;
  double angle2 = branch_angle - angle1;
  Eigen::Vector3d dir2 = dir * std::cos(angle2) - side_dir * std::sin(angle2);
  Eigen::Vector3d rand_dir2(ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5);
  Eigen::Vector3d side_dir2 = rand_dir2.cross(dir2).normalized();
  child2.tip = segments[root_id].tip + dir2 * k2;
  child2.attributes = segments[root_id].attributes;
  segments.push_back(child2);
  addSubTree(segments, (int)segments.size()-1, dir2, side_dir2, new_branch_length*k2,
    k1, k2, angle1, branch_angle, prune_length);
}


/// This method is used to grow or shrink a tree file by a number of years according to a very basic
/// model of tree growth.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  ray::DoubleArgument period(-1000, 3), length_rate(0.0001, 1000.0), width_rate(0.001, 10.0), prune_length_argument(0.001, 100.0);
  ray::TextArgument years("years");
  ray::OptionalFlagArgument shed_option("shed", 's');
  ray::OptionalKeyValueArgument length_option("length_rate", 'l', &length_rate);
  ray::OptionalKeyValueArgument prune_length_option("prune_length", 'l', &prune_length_argument);

  const bool parsed =
    ray::parseCommandLine(argc, argv, { &forest_file, &period, &years }, { &length_option, &shed_option, &prune_length_option });
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
    std::cout << "grow only works on tree structures, not trunks-only files" << std::endl;
    usage();
  }
  ray::ForestStructure grown_forest;
  const double len_rate = length_option.isSet() ? length_rate.value() : 0.3;
  const double length_growth = len_rate * period.value();

  if (period.value() > 0.0)
  {

    /// Information we need, per-tree:
    // 1. taper  (get length of tree, and radius at base)
    // 2. branch angle
    // 3. branch count to length exponent
    // 4. dominance
    auto &at_names = forest.trees[0].treeAttributeNames();
    auto &root_at_names = forest.trees[0].attributeNames();
    const int dimension_id = static_cast<int>(std::find(at_names.begin(), at_names.end(), "dimension") - at_names.begin());
    const int angle_id = static_cast<int>(std::find(root_at_names.begin(), root_at_names.end(), "angle") - root_at_names.begin());
    const int length_id = static_cast<int>(std::find(root_at_names.begin(), root_at_names.end(), "length") - root_at_names.begin());
    const int dominance_id = static_cast<int>(std::find(root_at_names.begin(), root_at_names.end(), "dominance") - root_at_names.begin());
    if (dimension_id == (int)at_names.size() || angle_id == (int)root_at_names.size() || 
        length_id == (int)root_at_names.size() || dominance_id == (int)root_at_names.size())
    {
      std::cout << "Error: cannot find required fields in file. Make sure to use the _info.txt file from the treeinfo command" << std::endl;
      usage();
    }

    // 1. store and retrieve prune_length from file
    // 2. for each singular branch, look at the perfect procedural tree at +growth, also pruned to prune_length. This uses dimension and dominance to work out split ratio up length
    // 3. split this branch at exact location based on linear interpolation of segments
    // 4. after this, drop off all the excess branches
    // 5. now grow the radius
    // 6. finally, re-adjust the segment lengths to have the same rough ratio of radius to length. Don't move the branch points
    const double prune_length = prune_length_option.isSet() ? prune_length_argument.value() : 1.0;

    for (auto &tree : forest.trees)
    {
      std::vector<int> children(tree.segments().size(), 0);
      for (size_t j = 0; j<tree.segments().size(); j++)
      {
        int par = tree.segments()[j].parent_id;
        if (par != -1)
        {
          children[par]++;
        }
      }
      auto &root = tree.segments()[0];
      double dimension = tree.treeAttributes()[dimension_id];
      double branch_angle = root.attributes[angle_id] * ray::kPi/180.0;
      double radius = root.radius;
      double length = root.attributes[length_id];
      double dominance = root.attributes[dominance_id];

      const double radius_growth = length_growth * radius / length;

      // convet dimension to the average downscale at each branch point
      const double k = std::pow(2.0, -1.0/dimension);
      // use dominance to work out the large and small downscale...
      // d1 * d2 = k*k
      // dominance = -1.0 + 2.0 * sqr(max_rad) / (sqr(max_rad) + sqr(second_max_rad));
      double area_ratio = (dominance + 1.0) / 2.0; // big child area per parent area
      double d1 = std::sqrt(area_ratio); // big child radius for unit parent radius
      double d2 = std::sqrt(1.0 - area_ratio); // small child radius for unit parent radius
      // if c^2 d1 d2 = k^2
      double d_scale = k / std::sqrt(d1 * d2);
      std::cout << "d1: " << d1 << ", d2: " << d2 << ", d_scale: " << d_scale << std::endl;
      double k1 = d1 * d_scale;
      double k2 = d2 * d_scale;

      // what are branch angles 1 and 2? We know the dominance and overall branch angle...
      // angle1 + angle2 = branch_angle
      // tan(angle1)/rad2^2 = tan(angle2)/rad1^2
      // tan(angle1)/tan(angle2) = (rad2 / rad1)^2
      double angle1 = branch_angle/2.0;
      for (int i = 0; i<20; i++) // no closed form expression, so iterate
      {
        angle1 = std::atan( std::tan(branch_angle - angle1) * ray::sqr(k2/k1));
      }  
      std::cout << "dominant angle: " << angle1 << ", total angle: " << branch_angle << std::endl;

      // 1. add subtrees at each leaf point....
      size_t num_segs = tree.segments().size();
      for (size_t i = 0; i<num_segs; i++)
      {
        auto &segments = tree.segments();
        auto &segment = segments[i];
        if (children[i] == 0) // a leaf
        {
          // extend the branch
          Eigen::Vector3d dir = segment.tip - segments[segment.parent_id].tip;
          double tip_length = dir.norm();
          dir /= tip_length;
          
          // now add a subtree here
          double initial_branch_length = tip_length;
          // b. new branch length
          double new_branch_length = initial_branch_length + prune_length + length_growth;

          Eigen::Vector3d random_dir(ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5);
          Eigen::Vector3d side_dir = dir.cross(random_dir).normalized();

          addSubTree(segments, i, dir, side_dir, new_branch_length,
            k1, k2, angle1, branch_angle, prune_length);          
        }
      }


      // 2. get length to end for each sub-branch, and add to a list in order to sort
      if (shed_option.isSet())
      {
        struct ListNode
        {
          int segment_id;
          double distance_to_end;
        };
        std::vector<ListNode> nodes;
        for (size_t i = 0; i<num_segs; i++)
        {
          auto &segment = tree.segments()[i];
          int parent = segment.parent_id;
          if (parent == -1 || children[parent] > 1) // condition for a sub-branch root
          {
            ListNode node;
            node.segment_id = (int)i;
            node.distance_to_end = segment.attributes[length_id];
            nodes.push_back(node);
          }
        }
        std::sort(nodes.begin(), nodes.end(), [](const ListNode &n1, const ListNode &n2) -> bool { return n1.distance_to_end > n2.distance_to_end; });
        // 3. calculate how much pruning should be done based on dimension

        // 4. go through nodes from longest to shortest, pruning out segments that don't fit the required power law 
      }
      for (auto &segment : tree.segments())
      {
        segment.radius = segment.radius + radius_growth;
      }
      // Lastly, we need to iterate through the segments and return them to roughly the original cylinder width to length ratio. Otherwise we'll get lots of short fat cylinders
    }
    grown_forest = forest;
  }
  else
  {
    ray::ForestStructure pruned_forest;
    tree::pruneLength(forest, -length_growth, pruned_forest);
    if (pruned_forest.trees.empty())
    {
      std::cout << "Warning: no trees left after shrinking. No file saved." << std::endl;
      return 1;
    }

    const double minimum_branch_diameter = 0.001;
    tree::pruneDiameter(pruned_forest, minimum_branch_diameter, grown_forest);
  }

  if (grown_forest.trees.empty())
  {
    std::cout << "Warning: no trees left after pruning. No file saved." << std::endl;
  }
  else
  {
    grown_forest.save(forest_file.nameStub() + "_grown.txt");
  }
  return 0;
}
