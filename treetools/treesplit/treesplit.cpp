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
  ray::Vector3dArgument plane, colour, box;
  ray::KeyValueChoice choice({ "plane", "colour", "radius", "box" }, { &plane, &colour, &radius, &box });

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
  // if splitting around an attribute in the tree file
  if (attribute_tree_format || attribute_trunk_format)
  {
    auto &att = attribute_tree_format ? forest.trees[0].treeAttributeNames() : forest.trees[0].attributeNames();;
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

  forest_in.save(forest_file.nameStub() + "_inside.txt");
  forest_out.save(forest_file.nameStub() + "_outside.txt");
  return 0;
}
