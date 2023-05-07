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
  std::cout << "treegrow forest.txt 1 years    - reduce the tree according to the rates, default values below." << std::endl;
  std::cout << "                    --length_rate 0.3   - expected branch length increase per year in m" << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method is used to grow or shrink a tree file by a number of years according to a very basic
/// model of tree growth.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  ray::DoubleArgument period(-1000, 3), length_rate(0.0001, 1000.0), width_rate(0.001, 10.0);
  ray::TextArgument years("years");
  ray::OptionalKeyValueArgument length_option("length_rate", 'l', &length_rate);

  const bool parsed =
    ray::parseCommandLine(argc, argv, { &forest_file, &period, &years }, { &length_option });
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
  const double len_rate = length_option.isSet() ? length_rate.value() : 0.3;
  const double length_growth = len_rate * period.value();

  /// Information we need, per-tree:
  // 1. taper  (get length of tree, and radius at base)
  // 2. branch angle
  // 3. branch count to length exponent
  // 4. dominance
  auto &at_names = forest.trees[0].treeAttributeNames();
  auto &root = forest.trees[0].segments()[0];
  auto &root_at_names = root.attributeNames();
//    int height_id = static_cast<int>(std::find(at_names.begin(), at_names.end(), "height") - at_names.begin());
  const int dimension_id = static_cast<int>(std::find(at_names.begin(), at_names.end(), "dimension") - at_names.begin());
  const int angle_id = static_cast<int>(std::find(root_at_names.begin(), root_at_names.end(), "angle") - root_at_names.begin());
  const int radius_id = static_cast<int>(std::find(root_at_names.begin(), root_at_names.end(), "radius") - root_at_names.begin());
  const int length_id = static_cast<int>(std::find(root_at_names.begin(), root_at_names.end(), "length") - root_at_names.begin());
  const int dominance_id = static_cast<int>(std::find(root_at_names.begin(), root_at_names.end(), "dominance") - root_at_names.begin());
  const int children_id = static_cast<int>(std::find(root_at_names.begin(), root_at_names.end(), "children") - root_at_names.begin());
  if (dimension_id == at_names.size() || angle_id == root_at_names.size() || radius_id == root_at_names.size() || length_id == root_at_names.size() || dominance_id == root_at_names.size())
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


//  for (int i = 0; i<static_cast<int>(period.value()); i++)
  {
    for (auto &tree : forest.trees)
    {
      auto &root = tree.segments()[0];
      double dimension = tree.attributes()[dimension_id];
      double branch_angle = root.attributes()[angle_id];
      double radius = root.attributes()[radius_id];
      double length = root.attributes()[length_id];
      double dominance = root.attributes()[dominance_id];

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

      // 1. add subtrees at each leaf point....
      for (size_t i = 0; i<tree.segments().size(); i++)
      {
        auto &segments = tree.segments();
        auto &segment = segments[i];
        if (segment.attributes[children_id] == 0) // a leaf
        {
          // extend the branch
          Eigen::Vector3d dir = segment.tip - segments[segment.parent_id].tip;
          double tip_length = dir.norm();
          dir /= norm;
          segment.tip = segments[segment.parent_id].tip + dir*(tip_length + length_growth); 
          
          // to add side branches we better back up and find the root of this branch...
          int root_id = i;
          std::vector<int> chain;
          chain.push_front(root_id);
          while (segments[root_id].parent_id != -1 && segments[root_id].attributes[children_id]<=1)
          {
            root_id = segments[root_id].parent_id;
            chain.push_front(root_id);
          }
          auto &root = segments[root_id];
          // now add a subtree here
          // a. get the length we have already...
          double initial_branch_length = root.attributes[length_id];
          // b. new branch length
          double new_branch_length = initial_branch_length + length_growth;
          // c. calculate split point along branch
          double split_length_along = new_branch_length * (1.0 - k1);
          
          double branch_length = 0.0;
          int par_id = segments[root_id].parent_id;
          Eigen::Vector3d last_tip = par_id==-1 ? segments[root_id].tip : segments[par_id].tip;
          double length_along = 0.0;
          for (int f = 0; f<(int)chain.size(); f++)
          {
            int j = chain[f];
            Eigen::Vector3d dir = segments[j].tip - last_tip;
            double length = dir.norm();
            dir /= norm;
            if (length_along+length > split_length_along)
            {
              // split this branch to add a child...
              // hmm, I think I assume that child indices are increasing
              double added_length = split_length_along - length_along;
              Eigen::Vector3d split_point = (last_tip + dir * added_length;
              
              ray::TreeStructure::BranchNode node;
              node.tip = segments[j].tip;
              node.radius = segments[j].radius;
              node.parent_id = j;
              segments[j].tip = split_point;
              if (f<(int)chain.size()-1)
              {
                segments[chain[f+1]].parent_id = segments.size(); // squeeze back inline
              }
              segments.push_back(node);

              // now add the short branch
              ray::TreeStructure::BranchNode offshoot;
              Eigen::Vector3d to_tip = segment.tip - split_point;
              Eigen::Vector3d tip_dir = to_tip.normalized();
              Eigen::Vector3d random_dir(ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5);
              Eigen::Vector3d side_dir = tip_dir.cross(random_dir).normalized();
              const double offshoot_angle = branch_angle;
              Eigen::Vector3d offshoot_dir = tip_dir*std::cos(offshoot_angle) + side_dir*std::sin(offshoot_angle);
              offshoot.tip = split_point + offshoot_dir * (new_branch_length * k2);
              offshoot.radius = segments[j].radius;
              offshoot.parent_id = j;
              segments.push_back(offshoot);
              break; // TODO: this whole thing needs to be a recursive function
            }
            length_along += length;
            last_tip = segments[j].tip;
          }
        }
      }


      // 2. get length to end for each sub-branch, and add to a list in order to sort
      struct ListNode
      {
        int segment_id;
        double distance_to_end;
      };
      std::vector<ListNode> nodes;
      for (size_t i = 0; i<tree.segments().size(); i++)
      {
        auto &segment = tree.segments()[i];
        int parent = segment.parent_id;
        if (parent_id == -1 || tree.segments()[parent].attributes[children_id] > 1) // condition for a sub-branch root
        {
          ListNode node;
          node.segment_id = i;
          node.distance_to_end = segment.attributes[length_id];
        }
      }
      std::sort(nodes.begin(), nodes.end(), [](const ListNode &n1, const ListNode &n2) -> bool { return n1.distance_to_end > n2.distance_to_end; });

      // 3. calculate how much pruning should be done based on dimension

      // 4. go through nodes from longest to shortest, pruning out segments that don't fit the required power law 

      // Finally, apply the radius growth
 
    }
  }

  // regardless of sign of the period, we scle the segment radii
  for (auto &tree : forest.trees)
  {
    for (auto &segment : tree.segments())
    {
      segment.radius = segment.radius + radius_growth;
    }
  }

  ray::ForestStructure grown_forest;
  // if the period is positive then we add length to the leaf branches
  if (period.value() > 0.0)
  {
    grown_forest = forest;
    // Just extend the leaves linearly
    for (size_t t = 0; t < forest.trees.size(); t++)
    {
      auto &tree = forest.trees[t];
      auto &new_tree = grown_forest.trees[t];
      std::vector<std::vector<int>> children(tree.segments().size());
      std::vector<double> min_length_from_leaf(tree.segments().size(), std::numeric_limits<double>::max());
      for (size_t i = 1; i < tree.segments().size(); i++)
      {
        children[tree.segments()[i].parent_id].push_back(static_cast<int>(i));
      }
      for (size_t i = 0; i < tree.segments().size(); i++)
      {
        if (children[i].size() == 0)  // a leaf, so ...
        {
          ray::TreeStructure::Segment segment = tree.segments()[i];
          segment.radius -= radius_growth;
          const int par = segment.parent_id;
          Eigen::Vector3d dir(0, 0, 1);
          if (par != -1)
          {
            dir = (tree.segments()[i].tip - tree.segments()[par].tip).normalized();
          }
          segment.tip += dir * length_growth;
          segment.parent_id = static_cast<int>(i);
          new_tree.segments().push_back(segment);
        }
      }
    }
  }
  // otherwise we are going back in time, so prune the length
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
