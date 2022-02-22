// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#include "treelib/treepruner.h"
#include <raylib/rayparse.h>
#include <raylib/raycloud.h>
#include <raylib/rayrenderer.h>
#include "raylib/raytreegen.h"
#include <cstdlib>
#include <iostream>

double sqr(double x){ return x*x; }

void usage(int exit_code = 1)
{
  std::cout << "Bulk information for the trees, plus per-branch and per-tree information saved out." << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treeinfo forest.txt - report tree information and save out to _info.txt file." << std::endl;
  std::cout << std::endl;
  std::cout << "Output file fields per segment (/ on root segment):" << std::endl;
  std::cout << "  volume: volume of segment  / total tree volume" << std::endl;
  std::cout << "  diameter: diameter of segment / max diameter on tree" << std::endl;
  std::cout << "  length: length of segment base to farthest leaf" << std::endl;
  std::cout << "  strength: d^0.75/l where d is diameter of segment and l is length from segment base to leaf" << std::endl;
  std::cout << "  min_strength: minimum strength between this segment and root" << std::endl;
  std::cout << "  dominance: a1/(a1+a2) for first and second largest child branches / mean for tree" << std::endl;
  std::cout << "  angle: angle between branches at each branch point / mean branch angle" << std::endl;
  std::cout << "Use treecolour 'field' to colour per-segment or treecolour trunk 'field' to colour per tree from root segment." << std::endl;
  std::cout << "Then use treemesh to render based on this colour output." << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  bool parsed = ray::parseCommandLine(argc, argv, {&forest_file});
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
  // attributes to estimate
  // 1. number of trees.
  // 2. volume.
  // 3. mass (using a mean tree density).
  // 4. min, mean and max trunk diameters.
  // 5. fractal distribution of trunk diameters?
  // 6. branch angles
  // 7. trunk dominance
  // 8. fractal distribution of branch diameters?
  // 9. min, mean and max tree heights.
  // 10. branch strengths
  std::cout << "Information" << std::endl;
  std::cout << std::endl;
  
  int num_attributes = (int)forest.trees[0].attributes().size();
  std::vector<std::string> new_attributes = {"volume", "diameter", "length", "strength", "min_strength", "dominance", "angle"};
  int volume_id = num_attributes+0;
  int diameter_id = num_attributes+1;
  int length_id = num_attributes+2;
  int strength_id = num_attributes+3;
  int min_strength_id = num_attributes+4;
  int dominance_id = num_attributes+5;
  int angle_id = num_attributes+6;
  auto &att = forest.trees[0].attributes();
  for (auto &new_at: new_attributes)
  {
    if (std::find(att.begin(), att.end(), new_at) != att.end())
    {
      std::cerr << "Error: cannot add info that is already present: " << new_at << std::endl;
      usage();
    }
  }
  // Fill in blank attributes across the whole structure
  for (auto &tree: forest.trees)
  {
    for (auto &new_at: new_attributes)
       tree.attributes().push_back(new_at);
    for (auto &segment: tree.segments())
    {
      for (size_t i = 0; i<new_attributes.size(); i++)
      {
        segment.attributes.push_back(0);
      }
    }
  }


  double min_branch_radius = 1e10;
  double total_volume = 0.0;
  double min_volume = 1e10, max_volume = -1e10;
  double total_diameter = 0.0;
  double min_diameter = 1e10, max_diameter = -1e10;
  double total_height = 0.0;
  double min_height = 1e10, max_height = -1e10;
  double total_strength = 0.0;
  double min_strength = 1e10, max_strength = -1e10;
  double total_dominance = 0.0;
  double min_dominance = 1e10, max_dominance = -1e10;
  double total_angle = 0.0;
  double min_angle = 1e10, max_angle = -1e10;
  int num_branched_trees = 0;
  for (auto &tree: forest.trees)
  {
    // get the children
    std::vector< std::vector<int> > children(tree.segments().size());
    for (size_t i = 1; i<tree.segments().size(); i++)
      children[tree.segments()[i].parent_id].push_back((int)i);

    double tree_dominance = 0.0;
    double tree_angle = 0.0;
    double total_weight = 0.0;
    for (size_t i = 1; i<tree.segments().size(); i++)
    {
      // for each leaf, iterate to trunk updating the maximum length...
      // TODO: would path length be better?
      if (children[i].empty()) // so it is a leaf
      {
        Eigen::Vector3d tip = tree.segments()[i].tip;
        int I = (int)i;
        int j = tree.segments()[I].parent_id;
        while (j != -1)
        {
          double dist = (tip - tree.segments()[j].tip).norm();
          double &length = tree.segments()[I].attributes[length_id];
          if (dist > length)
          {
            length = dist;
          }
          else
          {
            // TODO: this is a shortcut that won't work all the time, making the length a little approximate
            break; // remove to make it slower but exact
          }
          I = j;
          j = tree.segments()[I].parent_id;
        }
      }
      // if its a branch point then record how dominant the branching is
      else if (children[i].size() > 1)
      {
        double max_rad = -1.0;
        double second_max = -1.0;
        Eigen::Vector3d dir1(0,0,0), dir2(0,0,0);
        for (auto &child: children[i])
        {
          double rad = tree.segments()[child].radius;
          Eigen::Vector3d dir = tree.segments()[child].tip - tree.segments()[i].tip;
          if (children[child].size() == 1) // we go up a segment if we can, as the radius and angle will have settled better here
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
        double weight = sqr(max_rad) + sqr(second_max);
        double dominance = -1.0 + 2.0*sqr(max_rad) / weight;
        tree.segments()[i].attributes[dominance_id] = dominance;
        // now where do we spread this to?
        // if we spread to leaves then base will be empty, if we spread to parent then leave will be empty...
        tree_dominance += weight * dominance;
        total_weight += weight;
        double branch_angle = (180.0/ray::kPi) * std::atan2(dir1.cross(dir2).norm(), dir1.dot(dir2));
        tree.segments()[i].attributes[angle_id] = branch_angle;
        tree_angle += weight * branch_angle;
      }
    }
    for (auto &child: children[0])
      tree.segments()[0].attributes[length_id] = std::max(tree.segments()[0].attributes[length_id], tree.segments()[child].attributes[length_id]);
    
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
    }
    tree.segments()[0].attributes[dominance_id] = tree_dominance;
    tree.segments()[0].attributes[angle_id] = tree_angle;

    double tree_volume = 0.0;
    double tree_diameter = 0.0;
    for (size_t i = 1; i<tree.segments().size(); i++)
    {
      auto &branch = tree.segments()[i];
      min_branch_radius = std::min(min_branch_radius, branch.radius);
      double volume = ray::kPi * (branch.tip - tree.segments()[branch.parent_id].tip).norm() * branch.radius*branch.radius;
      branch.attributes[volume_id] = volume;
      branch.attributes[diameter_id] = 2.0 * branch.radius;
      tree_diameter = std::max(tree_diameter, branch.attributes[diameter_id]);
      tree_volume += volume;
      double denom = std::max(1e-10, branch.attributes[length_id]); // avoid a divide by 0
      branch.attributes[strength_id] = std::pow(branch.attributes[diameter_id], 3.0/4.0) / denom;
    }
    tree.segments()[0].attributes[volume_id] = tree_volume;
    total_volume += tree_volume;
    min_volume = std::min(min_volume, tree_volume);
    max_volume = std::max(max_volume, tree_volume);
    tree.segments()[0].attributes[diameter_id] = tree_diameter;
    total_diameter += tree_diameter;
    min_diameter = std::min(min_diameter, tree_diameter);
    max_diameter = std::max(max_diameter, tree_diameter);
    double tree_height = tree.segments()[0].attributes[length_id];
    total_height += tree_height;
    min_height = std::min(min_height, tree_height);
    max_height = std::max(max_height, tree_height);
    tree.segments()[0].attributes[strength_id] = std::pow(tree_diameter, 3.0/4.0) / tree_height;
    double tree_strength = tree.segments()[0].attributes[strength_id];
    total_strength += tree_strength;
    min_strength = std::min(min_strength, tree_strength);
    max_strength = std::max(max_strength, tree_strength);
  
    // alright, now how do we get the minimum strength from tip to root?
    for (auto &segment: tree.segments())
      segment.attributes[min_strength_id] = 1e10;
    std::vector<int> inds = children[0];
    for (size_t i = 0; i<inds.size(); i++)
    {
      int j = inds[i];
      auto &seg = tree.segments()[j];
      seg.attributes[min_strength_id] = std::min(seg.attributes[strength_id], tree.segments()[seg.parent_id].attributes[min_strength_id]);
      for (auto &child: children[j])
        inds.push_back(child);
    }
    tree.segments()[0].attributes[min_strength_id] = tree.segments()[0].attributes[strength_id]; // no different
  }
  

  std::cout << "Number of trees: " << forest.trees.size() << ", minimum branch diameter: " << 200.0 * min_branch_radius << " cm" << std::endl;
  std::cout << "Total volume of wood: " << total_volume << " m^3. Min/mean/max: " << min_volume << ", " << total_volume/(double)forest.trees.size() << ", " << max_volume << " m^3" << std::endl;
  std::cout << "Using example wood density of 0.5 Tonnes/m^3: Total mass of wood: " << 0.5 * total_volume << " Tonnes. Min/mean/max: " << 500.0*min_volume << ", " << 500.0*total_volume/(double)forest.trees.size() << ", " << 500.0*max_volume << " kg" << std::endl;
  std::cout << "Mean trunk diameter: " << total_diameter / (double)forest.trees.size() << " m. Min/max: " << min_diameter << ", " << max_diameter << " m" << std::endl;
  std::cout << "Mean tree height: " << total_height / (double)forest.trees.size() << " m. Min/max: " << min_height << ", " << max_height << " m" << std::endl;
  std::cout << "Mean trunk strength (diam^0.75/length): " << total_strength / (double)forest.trees.size() << ". Min/max: " << min_strength << ", " << max_strength << std::endl;
  std::cout << "Mean branch dominance (0 to 1): " << total_dominance / (double)num_branched_trees << ". Min/max: " << min_dominance << ", " << max_dominance << std::endl;
  std::cout << "Mean branch angle: " << total_angle / (double)num_branched_trees << " degrees. Min/max: " << min_angle << ", " << max_angle << " degrees" << std::endl;
  std::cout << std::endl;
  std::cout << "saving per-tree and per-segment data to file" << std::endl;
  forest.save(forest_file.nameStub() + "_info.txt");
  return 1;
}


