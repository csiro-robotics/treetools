// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#include <raylib/rayforeststructure.h>
#include <raylib/raytreegen.h>
#include <raylib/rayparse.h>
#include <raylib/raycloud.h>
#include <raylib/rayrenderer.h>
#include "raylib/raytreegen.h"
#include <cstdlib>
#include <iostream>

void usage(int exit_code = 1)
{
  std::cout << "Split a tree file around a criterion" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treesplit forest.txt radius 0.1   - split around tree radius (or any other attribute)" << std::endl;
  std::cout << "                     plane 0,0,1  - split around horizontal plane at height 1" << std::endl;
  std::cout << "                     colour 0,0,1 - split around colour green value 1" << std::endl;
  std::cout << "                     box rx,ry,rz - split around a centred box of the given radii" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, attribute(false);
  ray::Vector3dArgument coord;

  ray::DoubleArgument value(0.0,10000.0), radius(0.0,10000.0);
  ray::Vector3dArgument plane, colour, box;
  ray::KeyValueChoice choice({"plane", "colour", "radius", "box"}, 
                             {&plane, &colour, &radius, &box});

  bool parsed = ray::parseCommandLine(argc, argv, {&forest_file, &choice});
  bool attribute_format = false;
  if (!parsed)
    attribute_format = ray::parseCommandLine(argc, argv, {&forest_file, &attribute, &value});
  if (!parsed && !attribute_format)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }  

  ray::ForestStructure forest_in, forest_out;
  if (attribute_format)
  {
    auto &att = forest.trees[0].attributes();
    int attribute_id = -1;
    const auto &it = std::find(att.begin(), att.end(), attribute.name());
    if (it != att.end())
    {
      attribute_id = (int)(it - att.begin()); // we always assume that red is followed immediately by attributes green and blue
    }
    else
    {
      std::cerr << "Error: attribute " << attribute.name() << " was not found in " << forest_file.name() << std::endl;
      usage();
    }

    for (auto &tree: forest.trees)
    {
      if (tree.segments()[0].attributes[attribute_id] < value.value())
        forest_in.trees.push_back(tree);
      else
        forest_out.trees.push_back(tree);
    }
  }
  else if (choice.selectedKey() == "radius")
  {
    for (auto &tree: forest.trees)
    {
      if (tree.segments()[0].radius < radius.value())
        forest_in.trees.push_back(tree);
      else
        forest_out.trees.push_back(tree);
    }
  }
  else if (choice.selectedKey() == "plane")
  {
    Eigen::Vector3d plane_vec = plane.value() / plane.value().squaredNorm();
    for (auto &tree: forest.trees)
    {
      if (tree.segments()[0].tip.dot(plane_vec) < 1.0)
        forest_in.trees.push_back(tree);
      else
        forest_out.trees.push_back(tree);
    }
  }  
  else if (choice.selectedKey() == "colour")
  {
    Eigen::Vector3d colour_vec = colour.value() / colour.value().squaredNorm();
    for (auto &tree: forest.trees)
    {
      const auto &it = std::find(tree.attributes().begin(), tree.attributes().end(), "red");
      if (it != tree.attributes().end())
      {
        int red_index = (int)(it - tree.attributes().begin()); 
        auto &ats = tree.segments()[0].attributes;
        Eigen::Vector3d col(ats[red_index], ats[red_index+1], ats[red_index+2]); // we assume green, blue follow on consecutively 
        if (col.dot(colour_vec) < 1.0)
          forest_in.trees.push_back(tree);
        else
          forest_out.trees.push_back(tree);
      }
    }
  }
  else if (choice.selectedKey() == "box")
  {
    for (auto &tree: forest.trees)
    {
      auto &pos = tree.segments()[0].tip;
      if (std::abs(pos[0]) < box.value()[0] && std::abs(pos[1]) < box.value()[1] && std::abs(pos[2]) < box.value()[2])
        forest_in.trees.push_back(tree);
      else
        forest_out.trees.push_back(tree);
    }
  } 

  forest_in.save(forest_file.nameStub() + "_inside.txt");
  forest_out.save(forest_file.nameStub() + "_outside.txt");
}


