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
#include "treelib/treeutils.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Split a tree file around a criterion" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treesplit forest.txt radius 0.1        - split around trunk radius, or any other trunk user-attribute" << std::endl;
  std::cout << "                     tree height 0.1   - split around tree height (or any other whole-tree attribute)" << std::endl;
  std::cout << "                     plane 0,0,1       - split around horizontal plane at height 1" << std::endl;
  std::cout << "                     colour 0,0,1      - split around colour green value 1" << std::endl;
  std::cout << "                     box rx,ry,rz      - split around a centred box of the given radii" << std::endl;
  std::cout << "                     clusters 10       - approximate cluster size" << std::endl;
  std::cout << "                     per-tree          - one file per tree" << std::endl;
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
  ray::IntArgument cluster_size(0,1000);
  ray::Vector3dArgument plane, colour, box;
  ray::KeyValueChoice choice({ "plane", "colour", "radius", "box", "clusters"}, { &plane, &colour, &radius, &box, &cluster_size });

  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file, &choice });
  const bool split_per_tree = ray::parseCommandLine(argc, argv, { &forest_file, &per_tree_text });
  bool attribute_trunk_format = false;
  bool attribute_tree_format = false;
  if (!parsed && !split_per_tree)
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
  else if (choice.selectedKey() == "box")
  {
    for (auto &tree : forest.trees)
    {
      auto &pos = tree.segments()[0].tip;
      if (std::abs(pos[0]) < box.value()[0] && std::abs(pos[1]) < box.value()[1] && std::abs(pos[2]) < box.value()[2])
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
  else if (choice.selectedKey() == "clusters")
  {
    // one strategy: K means clustering.... 
    int K = (int)forest.trees.size() / cluster_size.value();
    // initialise some centres:
    std::vector<int> tree_ids(forest.trees.size());
    for (int i = 0; i<(int)tree_ids.size(); i++)
    {
      tree_ids[i] = i;
    }
    std::vector<Eigen::Vector2d> centres(K);
    std::cout << "first centres: ";
    for (int i = 0; i<K; i++)
    {
      int id = std::rand()%(int)tree_ids.size();
      tree_ids[id] = tree_ids.back();
      tree_ids.pop_back();
      auto &tree = forest.trees[tree_ids[id]];
      centres[i] = Eigen::Vector2d(tree.segments()[0].tip[0], tree.segments()[0].tip[1]);
      std::cout << centres[i].transpose() << ", ";
    }
    std::cout << std::endl;

    ray::ForestStructure cluster_template = forest;
    cluster_template.trees.clear();
    std::vector<ray::ForestStructure> tree_clusters(K, cluster_template);
    const int max_iterations = 10;
    for (int it = 0; it<max_iterations; it++)
    {
      std::vector<Eigen::Vector3d> new_centres(K, Eigen::Vector3d(0,0,0)); // horiz + weight
      // set new centres:
      for (auto &tree: forest.trees)
      {
        Eigen::Vector2d tree_pos = Eigen::Vector2d(tree.segments()[0].tip[0], tree.segments()[0].tip[1]);
        double min_dist = 1e20;
        Eigen::Vector2d min_pos(0,0);
        int min_k = 0;
        for (int i = 0; i<K; i++)
        {
          double dist = (tree_pos - centres[i]).squaredNorm();
          if (dist < min_dist)
          {
            min_dist = dist;
            min_pos = tree_pos;
            min_k = i;
          }
        }
        double w = tree.segments()[0].radius*tree.segments()[0].radius;
        new_centres[min_k] += w * Eigen::Vector3d(tree_pos[0], tree_pos[1], 1.0); // weight by cross-sectional area
        if (it == max_iterations-1)
        {
          tree_clusters[min_k].trees.push_back(tree);
        }
      }
      std::cout << "new centres: ";
      for (int i = 0; i<K; i++)
      {
        if (new_centres[i][2] > 0.0)
        {
          centres[i][0] = new_centres[i][0] / new_centres[i][2];
          centres[i][1] = new_centres[i][1] / new_centres[i][2];
          std::cout << centres[i].transpose() << ",  ";
        }
      }
      std::cout << std::endl;
    }
    int i = 0;
    for (auto &cluster: tree_clusters)
    {
      cluster.save(forest_file.nameStub() + "_cluster_" + std::to_string(i++) + ".txt");
    }
    // OK so after all iterations, we have lots of centres, 
    // I guess we then find the 
    return 0;
  }

  forest_in.save(forest_file.nameStub() + "_inside.txt");
  forest_out.save(forest_file.nameStub() + "_outside.txt");
  return 0;
}
