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
#include "treelib/treeinformation.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Placeholder method to grow or shrink the tree from the tips, using a linear model." << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treegrow forest.txt 1 years                 - age the tree by one year according to the length_rate" << std::endl;
  std::cout << "                    --length_rate 0.3       - expected branch length increase per year in m" << std::endl;
  std::cout << "                    --shed                  - shed branches to maintain branch length power law" << std::endl;
  std::cout << "                    --prune_length 1        - length from tip that reconstructed trees are pruned to, in m" << std::endl;
  std::cout << "                    --radius_growth_scale 1 - scale on the rate of radial growth" << std::endl;
  // clang-format on
  exit(exit_code);
}

void addSubTree(std::vector<ray::TreeStructure::Segment> &segments, int root_id, 
  Eigen::Vector3d dir, const Eigen::Vector3d &side_dir, double new_branch_length,
  double k1, double k2, double angle1, double branch_angle, double prune_length)
{
  const double uplift = 0.1; // causes branches to veer slightly upwards every bifurcation, as though seeking the sun a little
  dir = (dir + Eigen::Vector3d(0,0,uplift)).normalized();
  double bifurcate_distance = new_branch_length*(1.0 - k1);
  int par_id = segments[root_id].parent_id;
  if (new_branch_length*k2 < prune_length)
  {
    segments[root_id].tip = segments[par_id].tip + dir * (new_branch_length - prune_length);
    return;
  }
  segments[root_id].tip = segments[par_id].tip + dir * bifurcate_distance;
  
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
  ray::DoubleArgument period(-100, 100), length_rate(0.0001, 1000.0, 0.3), prune_length_argument(0.001, 100.0, 1.0);
  ray::DoubleArgument radius_growth_scale(0.0, 100.0, 1.0);
  ray::TextArgument years("years");
  ray::OptionalFlagArgument shed_option("shed", 's');
  ray::OptionalKeyValueArgument length_option("length_rate", 'l', &length_rate);
  ray::OptionalKeyValueArgument prune_length_option("prune_length", 'p', &prune_length_argument);
  ray::OptionalKeyValueArgument radius_growth_scale_option("radius_growth_scale", 'r', &radius_growth_scale);

  const bool parsed =
    ray::parseCommandLine(argc, argv, { &forest_file, &period, &years }, { &length_option, &shed_option, &prune_length_option, &radius_growth_scale_option });
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
  const double len_rate = length_rate.value();
  const double length_growth = len_rate * period.value();
  const double prune_length = prune_length_argument.value();

  for (auto &tree : forest.trees)
  {
    std::vector<std::vector<int> > children(tree.segments().size());
    for (size_t j = 0; j<tree.segments().size(); j++)
    {
      int par = tree.segments()[j].parent_id;
      if (par != -1)
      {
        children[par].push_back((int)j);
      }
    }
    /// Information we need, per-tree:
    // 1. taper  (get length of tree, and radius at base)
    // 2. branch angle
    // 3. dominance
    // 4. dimension
    std::vector<double> angles, num_children, dominances, all_lengths;
    // all_lengths are from segment start to end, including prune_length
    tree::getBranchLengths(tree, children, all_lengths, prune_length); 
    double total_dominance, total_angle, total_weight;
    tree::getBifurcationProperties(tree, children, angles, dominances, num_children, 
      total_dominance, total_angle, total_weight);
    std::vector<double> branch_lengths; // just the branches, not the segments
    std::vector<int> branch_ids;
    for (size_t j = 0; j<children.size(); j++)
    {
      auto &segs = tree.segments();
      if (segs[j].parent_id == -1 || children[segs[j].parent_id].size() > 1) 
      {
        bool secondary = true;
        if (segs[j].parent_id > -1)
        {
          double max_rad = 0.0;
          for (auto &child_id: children[segs[j].parent_id])
          {
            max_rad = std::max(max_rad, segs[child_id].radius);
          }
          secondary = segs[j].radius < max_rad; // only include non-dominant branches
        }
        if (secondary) // only include non-dominant branches
        {
          branch_ids.push_back((int)j);
          branch_lengths.push_back(all_lengths[j]); 
        }
      }
    }
    double power_c, power_D, r2; // rank = c * length^-D
    tree::calculatePowerLaw(branch_lengths, power_c, power_D, r2); 
    // std::cout << branch_lengths.size() << " branches, with rank = " << power_c << " * L^" << power_D << " with confidence: " << r2 << std::endl;

    // The key analytics here:
    const double dimension = std::max(0.5, std::min(-power_D, 3.0));
    double dominance = total_dominance / total_weight;
    dominance *= 0.5; // because it looks bad it we don't reduce it!
    const double branch_angle = (total_angle / total_weight) * ray::kPi/180.0;
    const double trunk_radius = tree.segments()[0].radius;
    const double tree_length = all_lengths[0];
    // std::cout << "dimension: " << dimension << ", dominance: " << dominance << ", branch angle rads: " << branch_angle << ", trunk radius: " << trunk_radius << ", tree length: " << tree_length << std::endl;

    const double radius_growth = radius_growth_scale.value() * length_growth * trunk_radius / tree_length;

    if (period.value() > 0.0)
    {
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
      // std::cout << "d1: " << d1 << ", d2: " << d2 << ", d_scale: " << d_scale << std::endl;
      double k1 = d1 * d_scale;
      double k2 = d2 * d_scale;
      k1 = std::min(k1, 0.9); // stop them getting too extreme
      k2 = std::min(k2, 0.9);

      if (!shed_option.isSet()) // we grow the radius differently if not shedding, otherwise radius gets too thick
      {
        for (auto &segment: tree.segments())
        {
          segment.radius += radius_growth;
        }
      }
      // what are branch angles 1 and 2? We know the dominance and overall branch angle...
      // angle1 + angle2 = branch_angle
      // tan(angle1)/rad2^2 = tan(angle2)/rad1^2
      // tan(angle1)/tan(angle2) = (rad2 / rad1)^2
      double angle1 = branch_angle/2.0;
      for (int i = 0; i<20; i++) // no closed form expression, so iterate
      {
        angle1 = std::atan( std::tan(branch_angle - angle1) * ray::sqr(k2/k1));
      }  
      // std::cout << "dominant angle: " << angle1 << ", total angle: " << branch_angle << std::endl;
      size_t num_segs = tree.segments().size();
      // 1. add subtrees at each leaf point....
      for (size_t i = 0; i<num_segs; i++)
      {
        auto &segments = tree.segments();
        auto &segment = segments[i];
        if (children[i].empty()) // a leaf
        {
          // extend the branch
          Eigen::Vector3d dir = segment.tip - segments[segment.parent_id].tip;
          double tip_length = dir.norm();
          dir /= tip_length;
          
          // now add a subtree here
          double initial_branch_length = tip_length;
          // b. new branch length
          double new_branch_length = initial_branch_length + prune_length + length_growth;
          if (shed_option.isSet())
          {
            segment.radius += radius_growth;
          }

          Eigen::Vector3d random_dir(ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5, ray::randUniformDouble()-0.5);
          Eigen::Vector3d side_dir = dir.cross(random_dir).normalized();

          addSubTree(segments, (int)i, dir, side_dir, new_branch_length,
            k1, k2, angle1, branch_angle, prune_length);          
        }
      }

      // 2. get length to end for each sub-branch, and add to a list in order to sort
      if (shed_option.isSet())
      {
        // TODO: an improvement might be to update the branch_ids, children etc so the new branches are also shed. Or to grow the new branches afterwards,
        // with a k value adjusted by the number of leaf points.
        struct ListNode
        {
          int segment_id;
          double distance_to_end;
          int total_branches; // number of subbranches including itself
          int order;
        };
        std::vector<ListNode> nodes;
        for (auto &i: branch_ids)
        {
          ListNode node;
          node.segment_id = (int)i;
          node.distance_to_end = all_lengths[i] + length_growth;
          node.total_branches = 1;
          std::vector<int> child_list = {(int)i};
          for (size_t j = 0; j<child_list.size(); j++)
          {
            auto &kids = children[child_list[j]];
            if (kids.size() > 1)
            {
              node.total_branches += (int)kids.size();
            }
            if (kids.size() > 0)
            {
              child_list.insert(child_list.end(), kids.begin(), kids.end());
            }
          }
          nodes.push_back(node);
        }
        std::sort(nodes.begin(), nodes.end(), [](const ListNode &n1, const ListNode &n2) -> bool { return n1.distance_to_end > n2.distance_to_end; });
        for (int i = 0; i<(int)nodes.size(); i++)
        {
          nodes[i].order = i;
        }
        // 3. calculate how much pruning should be done based on dimension
//        const double L0 = tree_length; // TODO: this should probably be the D'th root of estimated k (in rank = kL^-D)
        const double L0 = std::pow(power_c, 1.0/dimension); // TODO: this should probably be the D'th root of estimated k (in rank = kL^-D)
        // std::cout << "main tree length: " << tree_length << ", mean tree length: " << L0 << std::endl;
        // old law:          rank = L0^D * L^-D   // so L0 (full tree length) is rank 1
        // now grow L...
        // grown reality:    rank = L0^D * (L-length_growth)^-D
        // expected new law: rank = (L0+length_growth)^D * L^-D 
        const double kexp = std::pow(L0 + length_growth, dimension);
        // final drop is rank of the smallest branch minus rankof this branch after growth....
        const double smallest_branch_length = nodes.back().distance_to_end;
   //     const double smallest_branch_rank = 1.0 + (double)nodes.size();
        const double smallest_branch_rank = kexp * std::pow(smallest_branch_length - length_growth, -dimension);
        const double smallest_branch_new_rank = kexp * std::pow(smallest_branch_length, -dimension);
        // std::cout << "smallest branch rank: " << smallest_branch_rank << " new expected rank: " << smallest_branch_new_rank << ", drop: " << smallest_branch_rank - smallest_branch_new_rank << std::endl;
        const int final_drop = std::max(0, (int)(smallest_branch_rank - smallest_branch_new_rank));

        for (int i = 1; i<(int)nodes.size()-1; i++) // start at 1 because I don't want it chopping the whole tree down
        {
          int j = i+1; // look at what happens if the next one up slides down
          double length = nodes[j].distance_to_end;
//          double rank = 1.0 + (double)j;
          double rank = kexp * std::pow(length-length_growth, -dimension) + (double)(j - nodes[j].order);
          double expected_rank = kexp * std::pow(length, -dimension);
          if (expected_rank < rank-1.0) // 0.5) // - 0.5 chooses the closest (could go below the line) but -1 is more conservative and keeps the ranks above the expected line
          {
            bool remove_this_node = false;
            if (nodes[i].total_branches < final_drop && nodes[j].total_branches < final_drop)
            {
              remove_this_node = tree.segments()[nodes[i].segment_id].tip[2] < tree.segments()[nodes[j].segment_id].tip[2];
            }
            else
            {
              remove_this_node = nodes[i].total_branches < final_drop;
            }
            // we want to remove branch i here, but we need to check if the number of branches in branch i is 
            // less than the final drop
            if (remove_this_node)
            {
              int node_seg_id = nodes[i].segment_id;
              // now we remove not only node i, but also all the other subbranch nodes
              for (int l = (int)nodes.size()-1; l>=i; l--) // TODO: this is O(n^3), find a more efficient way to do this
              {
                // for each node, follow the parent down, and if it reaches i then remove this node...
                int id = nodes[l].segment_id;
                while (id != -1 && id != node_seg_id)
                {
                  id = tree.segments()[id].parent_id;
                }
                if (id == node_seg_id)
                {
                  // remove node
                  nodes.erase(nodes.begin() + l);
                }
              }
              tree.segments()[node_seg_id].parent_id = -1; // that's all we need to do for segments, as reindex will do the rest
              i--; // if we are removing this node then we need to decrement i
            }
          }
        }
        // updating the radius isn't trivial... 
        for (size_t i = 0; i<num_segs; i++)
        {
          auto &segment = tree.segments()[i];
          if (children[i].empty()) // a leaf
          {
            double old_radius = segment.radius - radius_growth;
            if (old_radius < 0.0)
              std::cout << "bad! " << i << std::endl;
            double area_addition = ray::sqr(segment.radius) - ray::sqr(old_radius);
            for (int j = segment.parent_id; j != -1; j = tree.segments()[j].parent_id)
            {
              tree.segments()[j].radius = std::sqrt(ray::sqr(tree.segments()[j].radius) + area_addition); 
            }
          }
        }  
        // 4. go through nodes from longest to shortest, pruning out segments that don't fit the required power law 
        tree.reindex();
      }
      // Lastly, we need to iterate through the segments and return them to roughly the original cylinder width to length ratio. Otherwise we'll get lots of short fat cylinders
    }
    else
    {
      for (auto &segment : tree.segments())
      {
        segment.radius = segment.radius + radius_growth;
      }
    }
  }
  grown_forest = forest;
  if (period.value() <= 0.0)
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
    if (pruned_forest.trees.empty())
    {
      std::cout << "Warning: no trees left after shrinking. No file saved." << std::endl;
      return 1;
    }
  }

  grown_forest.save(forest_file.nameStub() + "_grown.txt");
  return 0;
}
