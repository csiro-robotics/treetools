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
#include "raylib/raytreestructure.h"
#include "treelib/treeinformation.h"
#include "treelib/treepruner.h"

double sqr(double x)
{
  return x * x;
}

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Bulk information for the trees, plus per-branch and per-tree information saved out." << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treeinfo forest.txt - report tree information and save out to _info.txt file." << std::endl;
  std::cout << std::endl;
  std::cout << "Output file fields per tree:" << std::endl;
  std::cout << "  height: height of tree" << std::endl;
  std::cout << "  crown_radius: approximate radius of crown" << std::endl;
  std::cout << "  dimension: dimension of branch lengths" << std::endl;
  std::cout << "  monocotal: how strongly the tree appears to be a palm" << std::endl;
  std::cout << "Output file fields per segment (/ on root segment):" << std::endl;
  std::cout << "  volume: volume of segment  / total tree volume" << std::endl;
  std::cout << "  diameter: diameter of segment / max diameter on tree" << std::endl;
  std::cout << "  length: length of segment base to farthest leaf" << std::endl;
  std::cout << "  strength: d^0.75/l where d is diameter of segment and l is length from segment base to leaf" << std::endl;
  std::cout << "  min_strength: minimum strength between this segment and root" << std::endl;
  std::cout << "  dominance: a1/(a1+a2) for first and second largest child branches / mean for tree" << std::endl;
  std::cout << "  angle: angle between branches at each branch point / mean branch angle" << std::endl;
  std::cout << "  bend: bend of main trunk (standard deviation from straight line / length)" << std::endl;
  std::cout << "  children: number of children per branch / mean for tree" << std::endl;
  std::cout << "Use treecolour 'field' to colour per-segment or treecolour trunk 'field' to colour per tree from root segment." << std::endl;
  std::cout << "Then use treemesh to render based on this colour output." << std::endl;
  // clang-format on
  exit(exit_code);
}

void printAttributes(const ray::ForestStructure &forest, std::vector<std::string> &tree_att, int num_tree_attributes, 
                     std::vector<std::string> &att, int num_attributes)
{
  if (num_tree_attributes > 0)
  {
    std::cout << "Additional tree attributes mean, min, max:" << std::endl;
    for (int i = 0; i < num_tree_attributes; i++)
    {
      std::cout << "\t" << tree_att[i] << ":";
      for (int j = 0; j < 30 - static_cast<int>(tree_att[i].length()); j++)
      {
        std::cout << " ";
      }
      double value = 0.0;
      double num = 0.0;
      double max_val = std::numeric_limits<double>::lowest();
      double min_val = std::numeric_limits<double>::max();
      for (auto &tree : forest.trees)
      {
        const double val = tree.treeAttributes()[i];
        max_val = std::max(max_val, val);
        min_val = std::min(min_val, val);
        value += val;
        num++;
      }
      std::cout << value / num << ",\t" << min_val << ",\t" << max_val << std::endl;
    }
    std::cout << std::endl;
  }

  if (num_attributes > 0)
  {
    std::cout << "Additional branch segment attributes mean, min, max:" << std::endl;
    for (int i = 0; i < num_attributes; i++)
    {
      std::cout << "\t" << att[i] << ":";
      for (int j = 0; j < 30 - static_cast<int>(att[i].length()); j++)
      {
        std::cout << " ";
      }
      double value = 0.0;
      double num = 0.0;
      double max_val = std::numeric_limits<double>::lowest();
      double min_val = std::numeric_limits<double>::max();
      for (auto &tree : forest.trees)
      {
        for (auto &segment : tree.segments())
        {
          const double val = segment.attributes[i];
          max_val = std::max(max_val, val);
          min_val = std::min(min_val, val);
          value += val;
          num++;
        }
      }
      std::cout << value / num << ",\t" << min_val << ",\t" << max_val << std::endl;
    }
    std::cout << std::endl;
  }
}

/// This method analayses and outputs statistical information on the specified tree file.
/// This includes bulk measures for the whole file, and also outputs a tree file with attributes added
/// which are measures on a per-section and per-tree basis. Treecolour can then be used to visualise these
/// more localised statistics. The method also outputs several (.svg) graphs of branch frequency
/// relationships
int main(int argc, char *argv[])
{
  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout.precision(3);
  ray::FileArgument forest_file;
  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file });
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
    std::cout << "info only works on tree structures, not trunks-only files" << std::endl;
    usage();
  }
  // attributes to estimate
  // 5. fractal distribution of trunk diameters?
  // 8. fractal distribution of branch diameters?
  std::cout << "Information" << std::endl;
  std::cout << std::endl;


  const int num_tree_attributes = static_cast<int>(forest.trees[0].treeAttributeNames().size());
  const std::vector<std::string> new_tree_attributes = { "height", "crown_radius", "dimension", "monocotal" };
  const int height_id = num_tree_attributes + 0;
  const int crown_radius_id = num_tree_attributes + 1;
  const int dimension_id = num_tree_attributes + 2;
  const int monocotal_id = num_tree_attributes + 3;
  auto &tree_att = forest.trees[0].treeAttributeNames();
  for (auto &new_at : new_tree_attributes)
  {
    if (std::find(tree_att.begin(), tree_att.end(), new_at) != tree_att.end())
    {
      std::cerr << "Error: cannot add info that is already present in tree attributes: " << new_at << std::endl;
      usage();
    }
  }

  const int num_attributes = static_cast<int>(forest.trees[0].attributeNames().size());
  const std::vector<std::string> new_attributes = { "volume",    "diameter", "length", "strength", "min_strength",
                                                    "dominance", "angle",    "bend",   "children" };
  const int volume_id = num_attributes + 0;
  const int diameter_id = num_attributes + 1;
  const int length_id = num_attributes + 2;
  const int strength_id = num_attributes + 3;
  const int min_strength_id = num_attributes + 4;
  const int dominance_id = num_attributes + 5;
  const int angle_id = num_attributes + 6;
  const int bend_id = num_attributes + 7;
  const int children_id = num_attributes + 8;

  auto &att = forest.trees[0].attributeNames();
  for (auto &new_at : new_attributes)
  {
    if (std::find(att.begin(), att.end(), new_at) != att.end())
    {
      std::cerr << "Error: cannot add info that is already present: " << new_at << std::endl;
      usage();
    }
  }

  printAttributes(forest, tree_att, num_tree_attributes, att, num_attributes);

  // Fill in blank attributes across the whole structure
  double min_branch_radius = std::numeric_limits<double>::max();
  double max_branch_radius = std::numeric_limits<double>::lowest();
  double total_branch_radius = 0.0;
  int num_total = 0;
  for (auto &tree : forest.trees)
  {
    for (auto &new_at : new_tree_attributes)
    {
      tree.treeAttributeNames().push_back(new_at);
      tree.treeAttributes().push_back(0.0);
    }
    for (auto &new_at : new_attributes)
    {
      tree.attributeNames().push_back(new_at);
    }
    for (auto &segment : tree.segments())
    {
      min_branch_radius = std::min(min_branch_radius, segment.radius);
      max_branch_radius = std::max(max_branch_radius, segment.radius);
      total_branch_radius += segment.radius;
      num_total++;
      for (size_t i = 0; i < new_attributes.size(); i++)
      {
        segment.attributes.push_back(0);
      }
    }
  }
  const double broken_diameter = 0.6;  // larger than this is considered a broken branch and not extended
  const double taper_ratio = 140.0;

  double total_volume = 0.0;
  double min_volume = std::numeric_limits<double>::max(), max_volume = std::numeric_limits<double>::lowest();
  double total_diameter = 0.0;
  double min_diameter = std::numeric_limits<double>::max(), max_diameter = std::numeric_limits<double>::lowest();
  double total_height = 0.0;
  double min_height = std::numeric_limits<double>::max(), max_height = std::numeric_limits<double>::lowest();
  double total_strength = 0.0;
  double min_strength = std::numeric_limits<double>::max(), max_strength = std::numeric_limits<double>::lowest();
  double total_dominance = 0.0;
  double min_dominance = std::numeric_limits<double>::max(), max_dominance = std::numeric_limits<double>::lowest();
  double total_angle = 0.0;
  double min_angle = std::numeric_limits<double>::max(), max_angle = std::numeric_limits<double>::lowest();
  double total_bend = 0.0;
  double min_bend = std::numeric_limits<double>::max(), max_bend = std::numeric_limits<double>::lowest();
  double total_children = 0.0;
  double min_children = std::numeric_limits<double>::max(), max_children = std::numeric_limits<double>::lowest();
  double total_dimension = 0.0;
  double min_dimension = std::numeric_limits<double>::max(), max_dimension = std::numeric_limits<double>::lowest();
  double total_crown_radius = 0.0;
  double min_crown_radius = std::numeric_limits<double>::max(), max_crown_radius = std::numeric_limits<double>::lowest();
  int num_stat_trees = 0;  // used for dimension values

  int num_branched_trees = 0;
  int num_branches = 0;
  std::vector<double> tree_lengths;
  for (auto &tree : forest.trees)
  {
    // get the children
    std::vector<std::vector<int>> children(tree.segments().size());
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      children[tree.segments()[i].parent_id].push_back(static_cast<int>(i));
    }

    Eigen::Vector3d min_bound = tree.segments()[0].tip;
    Eigen::Vector3d max_bound = tree.segments()[0].tip;
    double tree_dominance = 0.0;
    double tree_angle = 0.0;
    double total_weight = 0.0;
    double tree_children = 0.0;
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      min_bound = ray::minVector(min_bound, tree.segments()[i].tip);
      max_bound = ray::maxVector(max_bound, tree.segments()[i].tip);
      // for each leaf, iterate to trunk updating the maximum length...
      if (children[i].empty())  // so it is a leaf
      {
        const double extension =
          2.0 * tree.segments()[i].radius > broken_diameter ? 0.0 : (taper_ratio * tree.segments()[i].radius);
        int I = static_cast<int>(i);
        int j = tree.segments()[I].parent_id;
        tree.segments()[I].attributes[length_id] = extension;
        int child = I;
        while (j != -1)
        {
          const double dist =
            tree.segments()[child].attributes[length_id] + (tree.segments()[I].tip - tree.segments()[j].tip).norm();
          double &length = tree.segments()[I].attributes[length_id];
          if (dist > length)
          {
            length = dist;
          }
          else
          {
            break;
          }
          child = I;
          I = j;
          j = tree.segments()[I].parent_id;
        }
      }
      // if its a branch point then record how dominant the branching is
      else if (children[i].size() > 1)
      {
        double max_rad = -1.0;
        double second_max = -1.0;
        Eigen::Vector3d dir1(0, 0, 0), dir2(0, 0, 0);
        for (auto &child : children[i])
        {
          double rad = tree.segments()[child].radius;
          Eigen::Vector3d dir = tree.segments()[child].tip - tree.segments()[i].tip;
          // we go up a segment if we can, as the radius and angle will have settled better here
          if (children[child].size() == 1)
          {
            rad = tree.segments()[children[child][0]].radius;
            dir = tree.segments()[children[child][0]].tip - tree.segments()[child].tip;
          }
          if (rad > max_rad)
          {
            second_max = max_rad;
            max_rad = rad;
            dir2 = dir1;
            dir1 = dir;
          }
          else if (rad > second_max)
          {
            second_max = rad;
            dir2 = dir;
          }
        }
        const double weight = sqr(max_rad) + sqr(second_max);
        const double dominance = -1.0 + 2.0 * sqr(max_rad) / weight;
        tree.segments()[i].attributes[dominance_id] = dominance;
        // now where do we spread this to?
        // if we spread to leaves then base will be empty, if we spread to parent then leave will be empty...
        tree_dominance += weight * dominance;
        total_weight += weight;
        const double branch_angle = (180.0 / ray::kPi) * std::atan2(dir1.cross(dir2).norm(), dir1.dot(dir2));
        tree.segments()[i].attributes[angle_id] = branch_angle;
        tree_angle += weight * branch_angle;
        tree.segments()[i].attributes[children_id] = static_cast<double>(children[i].size());
        tree_children += weight * static_cast<double>(children[i].size());
      }
    }
    for (auto &child : children[0])
    {
      tree.segments()[0].attributes[length_id] =
        std::max(tree.segments()[0].attributes[length_id], tree.segments()[child].attributes[length_id]);
    }
    if (children[0].size() > 0)
    {
      tree.segments()[0].attributes[children_id] = static_cast<double>(children[0].size());
    }
    tree::setTrunkBend(tree, children, bend_id, length_id);
    tree::setMonocotal(tree, children, monocotal_id);

    std::vector<double> lengths;
    int num_leaves = 0;
    for (auto &seg : tree.segments())
    {
      if (seg.attributes[children_id] > 1)
      {
        lengths.push_back(seg.attributes[length_id]);
      }
      else if (seg.attributes[children_id] == 0)
      {
        num_leaves++;
      }
    }
    const int min_branch_count = 6;  // can't do any reasonable stats with fewer than this number of branches
    num_branches += num_leaves + static_cast<int>(lengths.size());
    if (lengths.size() >= min_branch_count)
    {
      double c, d, r2;
      tree::calculatePowerLaw(lengths, c, d, r2);
      const double tree_dimension = std::min(-d, 3.0);
      tree.treeAttributes()[dimension_id] = tree_dimension;
      total_dimension += tree_dimension;
      max_dimension = std::max(max_dimension, tree_dimension);
      min_dimension = std::min(min_dimension, tree_dimension);
      num_stat_trees++;
    }

    // update per-tree values
    if (total_weight > 0.0)
    {
      tree_dominance /= total_weight;
      tree_angle /= total_weight;
      num_branched_trees++;
      total_dominance += tree_dominance;
      min_dominance = std::min(min_dominance, tree_dominance);
      max_dominance = std::max(max_dominance, tree_dominance);
      total_angle += tree_angle;
      min_angle = std::min(min_angle, tree_angle);
      max_angle = std::max(max_angle, tree_angle);
      tree_children /= total_weight;
      total_children += tree_children;
      min_children = std::min(min_children, tree_children);
      max_children = std::max(max_children, tree_children);
    }
    tree.segments()[0].attributes[dominance_id] = tree_dominance;
    tree.segments()[0].attributes[angle_id] = tree_angle;
    tree.segments()[0].attributes[children_id] = tree_children;

    double tree_volume = 0.0;
    double tree_diameter = 0.0;
    // update the volume, diameter and strength values in the tree
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      auto &branch = tree.segments()[i];
      const double volume =
        ray::kPi * (branch.tip - tree.segments()[branch.parent_id].tip).norm() * branch.radius * branch.radius;
      branch.attributes[volume_id] = volume;
      branch.attributes[diameter_id] = 2.0 * branch.radius;
      tree_diameter = std::max(tree_diameter, branch.attributes[diameter_id]);
      tree_volume += volume;
      const double denom = std::max(std::numeric_limits<double>::min(), branch.attributes[length_id]);  // avoid a divide by 0
      branch.attributes[strength_id] = std::pow(branch.attributes[diameter_id], 3.0 / 4.0) / denom;
    }
    // update whole-tree and tree root attributes
    tree.segments()[0].attributes[volume_id] = tree_volume;
    total_volume += tree_volume;
    min_volume = std::min(min_volume, tree_volume);
    max_volume = std::max(max_volume, tree_volume);
    tree.segments()[0].attributes[diameter_id] = tree_diameter;
    total_diameter += tree_diameter;
    min_diameter = std::min(min_diameter, tree_diameter);
    max_diameter = std::max(max_diameter, tree_diameter);
    double tree_height = max_bound[2] - tree.segments()[0].tip[2];
    tree.treeAttributes()[height_id] = tree_height;
    double crown_radius =
      ((max_bound[0] - min_bound[0]) + (max_bound[1] - min_bound[1])) / 2.0;  // mean of the bounding box extents
    total_crown_radius += crown_radius;
    min_crown_radius = std::min(min_crown_radius, crown_radius);
    max_crown_radius = std::max(max_crown_radius, crown_radius);
    tree.treeAttributes()[crown_radius_id] = crown_radius;
    total_height += tree_height;
    min_height = std::min(min_height, tree_height);
    max_height = std::max(max_height, tree_height);
    tree_lengths.push_back(tree.segments()[0].attributes[length_id]);
    tree.segments()[0].attributes[strength_id] = std::pow(tree_diameter, 3.0 / 4.0) 
      / std::max(std::numeric_limits<double>::min(), tree.segments()[0].attributes[length_id]);
    const double tree_strength = tree.segments()[0].attributes[strength_id];
    total_strength += tree_strength;
    min_strength = std::min(min_strength, tree_strength);
    max_strength = std::max(max_strength, tree_strength);
    const double tree_bend = tree.segments()[0].attributes[bend_id];
    total_bend += tree_bend;
    min_bend = std::min(min_bend, tree_bend);
    max_bend = std::max(max_bend, tree_bend);

    // alright, now we get the minimum strength from root to tip
    for (auto &segment : tree.segments())
    {
      segment.attributes[min_strength_id] = std::numeric_limits<double>::max();
    }
    std::vector<int> inds = children[0];
    for (size_t i = 0; i < inds.size(); i++)
    {
      int j = inds[i];
      auto &seg = tree.segments()[j];
      seg.attributes[min_strength_id] =
        std::min(seg.attributes[strength_id], tree.segments()[seg.parent_id].attributes[min_strength_id]);
      for (auto &child : children[j])
      {
        inds.push_back(child);
      }
    }
    tree.segments()[0].attributes[min_strength_id] = tree.segments()[0].attributes[strength_id];  // no different
  }
  std::cout << "Number of:" << std::endl;
  std::cout << "                  trees: " << forest.trees.size() << std::endl;
  std::cout << "                  branches: " << num_branches << std::endl;

  // Trunk power laws:
  std::vector<double> diameters;
  for (auto &tree : forest.trees)
  {
    diameters.push_back(2.0 * tree.segments()[0].radius);
  }
  double c, d, r2;
  tree::calculatePowerLaw(diameters, c, d, r2, "trunkwidth");
  std::cout << "    trunks wider than x: " << c << "x^" << d << "\t\twith correlation (r2) " << r2 << std::endl;
  tree::calculatePowerLaw(tree_lengths, c, d, r2, "treelength");
  std::cout << "    trees longer than l: " << c << "l^" << d << "\twith correlation (r2) " << r2 << std::endl;

  // Branch power laws:
  std::vector<double> lengths;
  for (auto &tree : forest.trees)
  {
    for (auto &seg : tree.segments())
    {
      if (seg.attributes[children_id] > 1.0)
        lengths.push_back(seg.attributes[length_id]);
    }
  }
  tree::calculatePowerLaw(lengths, c, d, r2, "branchlength");
  std::cout << " branches longer than l: " << c << "l^" << d << "\twith correlation (r2) " << r2 << std::endl;
  std::cout << std::endl;

  const double num_trees = static_cast<double>(forest.trees.size());
  std::cout << "Total:" << std::endl;
  std::cout << "              volume of wood: " << total_volume << " m^3.\tMean,min,max: " << total_volume / num_trees
            << ", " << min_volume << ", " << max_volume << " m^3" << std::endl;
  std::cout << " mass of wood (at 0.5 T/m^3): " << 0.5 * total_volume
            << " Tonnes.\tMean,min,max: " << 500.0 * total_volume / num_trees << ", " << 500.0 * min_volume << ", "
            << 500.0 * max_volume << " kg" << std::endl;
  std::cout << std::endl;

  std::cout << "Per-tree mean, min, max:" << std::endl;
  std::cout << "                trunk diameter (m): " << total_diameter / num_trees << ",\t" << min_diameter << ",\t"
            << max_diameter << std::endl;
  std::cout << "                   tree height (m): " << total_height / num_trees << ",\t" << min_height << ",\t"
            << max_height << std::endl;
  std::cout << "                  crown radius (m): " << total_crown_radius / num_trees << ",\t" << min_crown_radius
            << ",\t" << max_crown_radius << std::endl;
  std::cout << " trunk strength (diam^0.75/length): " << total_strength / num_trees << ",\t" << min_strength << ",\t"
            << max_strength << std::endl;
  std::cout << "         branch dominance (0 to 1): " << total_dominance / static_cast<double>(num_branched_trees)
            << ",\t" << min_dominance << ",\t" << max_dominance << std::endl;
  std::cout << "            branch angle (degrees): " << total_angle / static_cast<double>(num_branched_trees) << ",\t"
            << min_angle << ",\t" << max_angle << std::endl;
  std::cout << "              trunk bend (degrees): " << total_bend / num_trees << ",\t" << min_bend << ",\t"
            << max_bend << std::endl;
  std::cout << "               children per branch: " << total_children / static_cast<double>(num_branched_trees)
            << ",\t" << min_children << ",\t" << max_children << std::endl;
  std::cout << "          dimension (w.r.t length): " << total_dimension / static_cast<double>(num_stat_trees) << ",\t"
            << min_dimension << ",\t" << max_dimension << std::endl;
  std::cout << std::endl;

  std::cout << "Per-branch mean, min, max:" << std::endl;
  std::cout << "                     diameter (cm): " << 200.0 * total_branch_radius / static_cast<double>(num_total)
            << ",\t" << 200.0 * min_branch_radius << ",\t" << 200.0 * max_branch_radius << std::endl;
  std::cout << std::endl;

  std::cout << "saving per-tree and per-segment data to file" << std::endl;
  forest.save(forest_file.nameStub() + "_info.txt");
  return 0;
}
