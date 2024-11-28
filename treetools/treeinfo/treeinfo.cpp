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
  std::cout << "treeinfo forest.txt        - report tree information and save out to _info.txt file." << std::endl;
  std::cout << "          --branch_data    - creates a branch number, segment_length, branch order number, extension and position in branch integers per-segment" << std::endl;
  std::cout << "          --layer_height 5 - additional volume reporting per vertical layer" << std::endl;
  std::cout << "          --crop_length 1  - should reflect the value used in rayextract trees if you want full values for branch lengths" << std::endl;
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

/// @brief used for storing and printing the total results of different info metrics
struct Metrics
{
  struct Stats
  {
    Stats() : total(0.0), min(std::numeric_limits<double>::max()), max(std::numeric_limits<double>::lowest()) {}
    void update(double value)
    {
      total += value;
      min = std::min(min, value);
      max = std::max(max, value);      
    }
    double total;
    double min;
    double max;
  };
  Stats volume, DBH, height, strength, dominance, angle, bend, dimension, crown_radius, branch_radius, posx, posy;

  void print(size_t numtrees, int num_branched_trees, int num_stat_trees, int num_total)
  {
    const double num_trees = static_cast<double>(numtrees);
    // clang-format off
    std::cout << "Total:" << std::endl;
    std::cout << "              volume of wood: " << volume.total << " m^3.\tMean,min,max: " << volume.total / num_trees << ", " << volume.min << ", " << volume.max << " m^3" << std::endl;
    std::cout << " mass of wood (at 0.5 T/m^3): " << 0.5 * volume.total << " Tonnes.\tMean,min,max: " << 500.0 * volume.total / num_trees << ", " << 500.0 * volume.min << ", " << 500.0 * volume.max << " kg" << std::endl;
    std::cout << "                    location: " << posx.total/num_trees << ", " << posy.total/num_trees << ", min: " << posx.min << ", " << posy.min << ", max: " << posx.max << ", " << posy.max << std::endl; 
    std::cout << std::endl;
    std::cout << "Per-tree mean, min, max:" << std::endl;
    std::cout << "          trunk diameter (DBH) (m): " << DBH.total / num_trees << ",\t" << DBH.min << ",\t" << DBH.max << std::endl;
    std::cout << "                   tree height (m): " << height.total / num_trees << ",\t" << height.min << ",\t" << height.max << std::endl;
    std::cout << "                  crown radius (m): " << crown_radius.total / num_trees << ",\t" << crown_radius.min << ",\t" << crown_radius.max << std::endl;
    std::cout << " trunk strength (diam^0.75/length): " << strength.total / num_trees << ",\t" << strength.min << ",\t" << strength.max << std::endl;
    std::cout << "         branch dominance (0 to 1): " << dominance.total / static_cast<double>(num_branched_trees) << ",\t" << dominance.min << ",\t" << dominance.max << std::endl;
    std::cout << "            branch angle (degrees): " << angle.total / static_cast<double>(num_branched_trees) << ",\t" << angle.min << ",\t" << angle.max << std::endl;
    std::cout << "                trunk bend (ratio): " << bend.total / num_trees << ",\t" << bend.min << ",\t" << bend.max << std::endl;
    std::cout << "          dimension (w.r.t length): " << dimension.total / static_cast<double>(num_stat_trees) << ",\t" << dimension.min << ",\t" << dimension.max << std::endl;
    std::cout << std::endl;
    std::cout << "Per-branch mean, min, max:" << std::endl;
    std::cout << "                     diameter (cm): " << 200.0 * branch_radius.total / static_cast<double>(num_total) << ",\t" << 200.0 * branch_radius.min << ",\t" << 200.0 * branch_radius.max << std::endl;
    std::cout << std::endl;
    // clang-format on
  }
};

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
  ray::DoubleArgument layer_height(0, 100.0, 5.0), crop_length(0.0, 100.0, 1.0);
  ray::OptionalFlagArgument branch_data("branch_data", 'b');
  ray::OptionalKeyValueArgument layer_option("layer_height", 'l', &layer_height);
  ray::OptionalKeyValueArgument crop_length_option("crop_length", 'c', &crop_length);
  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file }, { &layer_option, &branch_data, &crop_length_option });
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

  if (layer_option.isSet() && layer_height.value() > 0.0)
  {
    double max_height = 0.0;
    for (auto &tree : forest.trees)
    {
      for (auto &segment: tree.segments())
      {
        max_height = std::max(max_height, segment.tip[2] - tree.segments()[0].tip[2]);
      }
    }
    int num_layers = (int)std::ceil(max_height / layer_height.value());
    std::cout << "Wood volume by " << layer_height.value() << " m layer from ground:" << std::endl;
    for (int i = 0; i<num_layers; i++)
    {
      double min_z = (double)i * layer_height.value();
      double max_z = (double)(i+1) * layer_height.value();
      double layer_volume = 0.0;
      for (auto &tree : forest.trees)
      {
        for (auto &segment: tree.segments())
        {     
          double minz = min_z + tree.segments()[0].tip[2];
          double maxz = max_z + tree.segments()[0].tip[2];
          // now crop the cylinder to this range.... 
          double cylinder_volume = ray::kPi * segment.radius * segment.radius * (segment.tip - tree.segments()[segment.parent_id].tip).norm();
          double top = segment.tip[2];
          double bottom = tree.segments()[segment.parent_id].tip[2];
          if (top < bottom)
          {
            std::swap(top, bottom);
          }
          if (top == bottom)
          {
            if (top < minz || top > maxz)
              cylinder_volume = 0.0;
          }
          else 
          {
            cylinder_volume *= std::max(0.0, std::min(top, maxz) - std::max(bottom, minz)) / (top - bottom);
          }
          layer_volume += cylinder_volume;
        }
      } 
      std::cout << "Layer " << i << ": " << layer_volume << " m^3" << std::endl;
    }
    std::cout << std::endl;
  }



  const int num_tree_attributes = static_cast<int>(forest.trees[0].treeAttributeNames().size());
  const std::vector<std::string> new_tree_attributes = { "height", "crown_radius", "dimension", "monocotal", "DBH", "bend", "branch_slope" };
  const int height_id = num_tree_attributes + 0;
  const int crown_radius_id = num_tree_attributes + 1;
  const int dimension_id = num_tree_attributes + 2;
  const int monocotal_id = num_tree_attributes + 3;
  const int DBH_id = num_tree_attributes + 4;
  const int bend_id = num_tree_attributes + 5;
  const int branch_slope_id = num_tree_attributes + 6;

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
  std::vector<std::string> new_attributes = { "volume",    "diameter", "length", "strength", "min_strength",
                                                    "dominance", "angle",    "children" };
  if (branch_data.isSet())
  {
    new_attributes.push_back("branch");
    new_attributes.push_back("branch_order");
    new_attributes.push_back("extension");
    new_attributes.push_back("pos_in_branch");
    new_attributes.push_back("segment_length");
  }
  const int volume_id = num_attributes + 0;
  const int diameter_id = num_attributes + 1;
  const int length_id = num_attributes + 2;
  const int strength_id = num_attributes + 3;
  const int min_strength_id = num_attributes + 4;
  const int dominance_id = num_attributes + 5;
  const int angle_id = num_attributes + 6;
  const int children_id = num_attributes + 7;
  // optional with branch_data
  const int branch_id = num_attributes + 8;
  const int branch_order_id = num_attributes + 9;
  const int extension_id = num_attributes + 10;
  const int pos_in_branch_id = num_attributes + 11;
  const int segment_length_id = num_attributes + 12;

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
  Metrics metrics;
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
    metrics.posx.update(tree.segments()[0].tip[0]);
    metrics.posy.update(tree.segments()[0].tip[1]);
    for (auto &segment : tree.segments())
    {
      metrics.branch_radius.update(segment.radius);
      num_total++;
      for (size_t i = 0; i < new_attributes.size(); i++)
      {
        segment.attributes.push_back(0);
      }
    }
  }
  const double prune_length = crop_length.value();

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
    if (branch_data.isSet())
    {
      // 1. get branch IDs:
      int branch_number = 1;
      std::vector<Eigen::Vector4i> ids(tree.segments().size(), Eigen::Vector4i(0,0,branch_number,0)); // seg id, branch order, branch, pos on branch
      for (size_t i = 0; i < tree.segments().size(); i++)
      {
        double max_score = -1;
        int largest_child = -1;
        Eigen::Vector4i id = ids[i];
        for (const auto &child : children[i])
        {
          // we pick the route which has the longer and wider branch
          double score = tree.segments()[child].radius;
          if (score > max_score)
          {
            max_score = score;
            largest_child = child;
          }
        }
        for (const auto &child : children[i])
        {
          Eigen::Vector4i data(child, id[1], id[2], id[3]+1); // seg id, branch order, branch, pos on branch
          if (child == largest_child)
          {
            tree.segments()[i].attributes[extension_id] = data[0];
          }
          else
          {
            data[1]++;
            data[2] = branch_number++;
            data[3] = 0;
          }
          tree.segments()[child].attributes[branch_order_id] = data[1];
          tree.segments()[child].attributes[branch_id] = data[2];
          tree.segments()[child].attributes[pos_in_branch_id] = data[3];
          ids[child] = data;            
        }
        auto &segment = tree.segments()[i];
        if (segment.parent_id != -1)
        {
          segment.attributes[segment_length_id] = (segment.tip - tree.segments()[segment.parent_id].tip).norm();
        }
      }
    }

    Eigen::Vector3d min_bound = tree.segments()[0].tip;
    Eigen::Vector3d max_bound = tree.segments()[0].tip;
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      min_bound = ray::minVector(min_bound, tree.segments()[i].tip);
      max_bound = ray::maxVector(max_bound, tree.segments()[i].tip);
    }
    std::vector<double> branch_lengths;
    tree::getBranchLengths(tree, children, branch_lengths, prune_length);
    for (size_t j = 0; j<branch_lengths.size(); j++)
    {
      tree.segments()[j].attributes[length_id] = branch_lengths[j];
    }
    double tree_dominance = 0.0;
    double tree_angle = 0.0;
    double total_weight = 0.0;
    std::vector<double> branch_angles, branch_dominances, branch_children;
    tree::getBifurcationProperties(tree, children, branch_angles, branch_dominances, branch_children, tree_dominance, tree_angle, total_weight);    
    for (size_t j = 0; j<branch_lengths.size(); j++)
    {
      tree.segments()[j].attributes[angle_id] = branch_angles[j];
      tree.segments()[j].attributes[dominance_id] = branch_dominances[j];
      tree.segments()[j].attributes[children_id] = branch_children[j];
    }
    if (children[0].size() > 0)
    {
      tree.segments()[0].attributes[children_id] = static_cast<double>(children[0].size());
    }
    tree::setTrunkBend(tree, children, bend_id, length_id, branch_slope_id);
    metrics.bend.update(tree.treeAttributes()[bend_id]);
    tree::setMonocotal(tree, children, monocotal_id);
    tree::setDBH(tree, children, DBH_id);
    metrics.DBH.update(tree.treeAttributes()[DBH_id]);

    std::vector<double> lengths;
    for (size_t j = 0; j<children.size(); j++)
    {
      auto &seg = tree.segments()[j];
      int par = seg.parent_id;
      if (par == -1 || children[par].size() > 1) 
      {
        lengths.push_back(seg.attributes[length_id]); // is the length even being set on this exact segment??
      }
    }
    const int min_branch_count = 6;  // can't do any reasonable stats with fewer than this number of branches
    num_branches += static_cast<int>(lengths.size());
    if (lengths.size() >= min_branch_count)
    {
      double c, d, r2;
      tree::calculatePowerLaw(lengths, c, d, r2);
      const double tree_dimension = std::min(-d, 3.0);
      tree.treeAttributes()[dimension_id] = tree_dimension;
      metrics.dimension.update(tree_dimension);
      num_stat_trees++;
    }

    // update per-tree values
    if (total_weight > 0.0)
    {
      tree_dominance /= total_weight;
      tree_angle /= total_weight;
      num_branched_trees++;
      metrics.dominance.update(tree_dominance);
      metrics.angle.update(tree_angle);
    }
    tree.segments()[0].attributes[dominance_id] = tree_dominance;
    tree.segments()[0].attributes[angle_id] = tree_angle;

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
    metrics.volume.update(tree_volume);
    tree.segments()[0].attributes[diameter_id] = tree_diameter;
    double tree_height = prune_length + max_bound[2] - tree.segments()[0].tip[2];
    tree.treeAttributes()[height_id] = tree_height;
    double crown_radius = prune_length + 
      ((max_bound[0] - min_bound[0]) + (max_bound[1] - min_bound[1])) / 2.0;  // mean of the bounding box extents
    metrics.crown_radius.update(crown_radius);
    tree.treeAttributes()[crown_radius_id] = crown_radius;
    metrics.height.update(tree_height);
    tree_lengths.push_back(tree.segments()[0].attributes[length_id]);
    tree.segments()[0].attributes[strength_id] = std::pow(tree_diameter, 3.0 / 4.0) 
      / std::max(std::numeric_limits<double>::min(), tree.segments()[0].attributes[length_id]);
    metrics.strength.update(tree.segments()[0].attributes[strength_id]);

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

  metrics.print(forest.trees.size(), num_branched_trees, num_stat_trees, num_total);
  std::cout << "saving per-tree and per-segment data to file" << std::endl;
  forest.save(forest_file.nameStub() + "_info.txt");
  return 0;
}
