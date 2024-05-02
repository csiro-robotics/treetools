// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <raylib/raycloud.h>
#include <raylib/rayforeststructure.h>
#include <raylib/rayparse.h>
#include <raylib/rayrenderer.h>
#include <raylib/raytreegen.h>
#include <cstdlib>
#include <iostream>
#include "raylib/raytreegen.h"
#include "raylib/extraction/rayclusters.h"
#include "treelib/treeutils.h"


void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Split a tree file around a criterion" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treesplit forest.txt radius 0.1         - split around trunk radius, or any other trunk user-attribute" << std::endl;
  std::cout << "                     tree height 0.1    - split around tree height (or any other whole-tree attribute)" << std::endl;
  std::cout << "                     plane 0,0,1        - split around horizontal plane at height 1" << std::endl;
  std::cout << "                     colour 0,0,1       - split around colour green value 1" << std::endl;
  std::cout << "                     box x,y rx,ry - split around an x,y,z centred box of the given radii" << std::endl;
  std::cout << "                     cluster_width 10   - split into clusters of this max diameter" << std::endl;
  std::cout << "                     per-tree           - one file per tree" << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method splits the tree file into two files on a per-tree basis, according to the specified
/// criterion.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, attribute(false);
  ray::TextArgument tree_text("tree"), trunk_text("trunk"), per_tree_text("per-tree");
  ray::Vector3dArgument coord;

  ray::DoubleArgument value(0.0, 10000.0), radius(0.0, 10000.0);
  ray::DoubleArgument cluster_size(0.0,1000.0);
  ray::Vector3dArgument plane, colour;
  ray::Vector2dArgument box_centre, box_radius;
  ray::TextArgument box_text("box");
  ray::KeyValueChoice choice({ "plane", "colour", "radius", "cluster_width"}, { &plane, &colour, &radius, &cluster_size });
  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file, &choice });
  const bool box_format = ray::parseCommandLine(argc, argv, { &forest_file, &box_text, &box_centre, &box_radius });
  const bool split_per_tree = ray::parseCommandLine(argc, argv, { &forest_file, &per_tree_text });
  bool attribute_trunk_format = false;
  bool attribute_tree_format = false;
  if (!parsed && !split_per_tree && !box_format)
  {
    // split around an attribute defined on the trunk (the base segment)
    attribute_trunk_format = ray::parseCommandLine(argc, argv, { &forest_file, &attribute, &value });
    // split around an attribute defined on the whole tree
    attribute_tree_format = ray::parseCommandLine(argc, argv, { &forest_file, &tree_text, &attribute, &value });
    if (!attribute_trunk_format && !attribute_tree_format)
    {
      usage();
    }
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }

  ray::ForestStructure forest_in, forest_out;
  forest_in.comments = forest.comments;
  forest_out.comments = forest.comments;
  // if splitting around an attribute in the tree file
  if (attribute_tree_format || attribute_trunk_format)
  {
    auto &att = attribute_tree_format ? forest.trees[0].treeAttributeNames() : forest.trees[0].attributeNames();
    int attribute_id = -1;
    const auto &it = std::find(att.begin(), att.end(), attribute.name());
    if (it != att.end())
    {
      // we always assume that red is followed immediately by attributes green and blue
      attribute_id = static_cast<int>(it - att.begin());
    }
    else
    {
      std::cerr << "Error: attribute " << attribute.name() << " was not found in " << forest_file.name() << std::endl;
      usage();
    }

    // do the split per-tree
    for (auto &tree : forest.trees)
    {
      if ((attribute_tree_format && tree.treeAttributes()[attribute_id] < value.value()) ||
          (attribute_trunk_format && tree.segments()[0].attributes[attribute_id] < value.value()))
      {
        forest_in.trees.push_back(tree);
      }
      else
      {
        forest_out.trees.push_back(tree);
      }
    }
  }
  // literally one file saved per tree
  else if (split_per_tree)
  {
    int i = 0;
    for (auto &tree : forest.trees)
    {
      i++;
      ray::ForestStructure new_tree;
      new_tree.trees.push_back(tree);
      new_tree.save(forest_file.nameStub() + "_" + std::to_string(i) + ".txt");
    }
    return 0;
  }
  // split based on a trunk radius threshold
  else if (choice.selectedKey() == "radius")
  {
    for (auto &tree : forest.trees)
    {
      if (tree.segments()[0].radius < radius.value())
      {
        forest_in.trees.push_back(tree);
      }
      else
      {
        forest_out.trees.push_back(tree);
      }
    }
  }
  // split around a user-defined plane
  else if (choice.selectedKey() == "plane")
  {
    const Eigen::Vector3d plane_vec = plane.value() / plane.value().squaredNorm();
    for (auto &tree : forest.trees)
    {
      if (tree.segments()[0].tip.dot(plane_vec) < 1.0)
      {
        forest_in.trees.push_back(tree);
      }
      else
      {
        forest_out.trees.push_back(tree);
      }
    }
  }
  // split around a plane in rgb space
  else if (choice.selectedKey() == "colour")
  {
    const Eigen::Vector3d colour_vec = colour.value() / colour.value().squaredNorm();
    for (auto &tree : forest.trees)
    {
      const auto &it = std::find(tree.attributeNames().begin(), tree.attributeNames().end(), "red");
      if (it != tree.attributeNames().end())
      {
        const int red_index = static_cast<int>(it - tree.attributeNames().begin());
        auto &ats = tree.segments()[0].attributes;
        const Eigen::Vector3d col(ats[red_index], ats[red_index + 1],
                                  ats[red_index + 2]);  // we assume green, blue follow on consecutively
        if (col.dot(colour_vec) < 1.0)
        {
          forest_in.trees.push_back(tree);
        }
        else
        {
          forest_out.trees.push_back(tree);
        }
      }
    }
  }
  // split inside/outside a box
  else if (box_format)
  {
    for (auto &tree : forest.trees)
    {
      Eigen::Vector2d b_centre = box_centre.value();
      Eigen::Vector2d b_radius = box_radius.value();
      auto &pos = tree.segments()[0].tip;
      if (std::abs(pos[0] - b_centre[0]) < b_radius[0] && std::abs(pos[1] - b_centre[1]) < b_radius[1])
      {
        forest_in.trees.push_back(tree);
      }
      else
      {
        forest_out.trees.push_back(tree);
      }
    }
  }
  // split into clusters of an approximate size
  else if (choice.selectedKey() == "cluster_width")
  {
    std::vector<Eigen::Vector3d> centres;
    for (auto &tree: forest.trees)
    {
      centres.push_back(tree.segments()[0].tip);
    }
    ray::ForestStructure cluster_template = forest;
    cluster_template.trees.clear();

    std::vector<std::vector<int> > point_clusters;
    double min_diameter = cluster_size.value(); // adjacent max distance
    double max_diameter = cluster_size.value(); // maximum width of cluster
    ray::generateClusters(point_clusters, centres, min_diameter, max_diameter);
    std::cout << "found " << point_clusters.size() << " clusters" << std::endl;
    std::vector<ray::ForestStructure> tree_clusters(point_clusters.size(), cluster_template);
    int i = 0;
    for (auto &cluster: point_clusters)
    {
      for (auto &id: cluster)
      {
        tree_clusters[i].trees.push_back(forest.trees[id]);
      }
      tree_clusters[i].save(forest_file.nameStub() + "_cluster_" + std::to_string(i) + ".txt");
      i++;
    }
    return 0;
  }

  forest_in.save(forest_file.nameStub() + "_inside.txt");
  forest_out.save(forest_file.nameStub() + "_outside.txt");
  return 0;
}
