// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#include <raylib/rayforestgen.h>
#include <raylib/raytreegen.h>
#include <raylib/rayparse.h>
#include <cstdlib>
#include <iostream>

void usage(int exit_code = 1)
{
  std::cout << "Create an example forest from the specified parameters" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treecreate tree 1      - create a single tree with the given random seed" << std::endl;
  std::cout << "           --random_factor 0.25   - degree of randomness in the construction" << std::endl;
  std::cout << "           --max_trunk_radius 0.2 - maximum trunk radius (or the radius for a single tree)" << std::endl;
  std::cout << "treecreate forest 1    - create a forest with the given random seed" << std::endl;
  std::cout << "           --width 20             - width of square section" << std::endl;
  std::cout << "           --dimension 2          - number of trees = radius^-dimension" << std::endl;
  std::cout << "           --tree_density 0.01    - number of mature trees per m^2" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::TextArgument tree_text("tree"), forest_text("forest");
  ray::DoubleArgument width(0.0001, 1000.0), max_trunk_radius(0.0001, 1000.0), dimension(0.0001, 10.0), tree_density(0.0001, 100.0);
  ray::IntArgument seed(0, 100.0);
  ray::DoubleArgument random_factor(0, 100.0);
  ray::OptionalKeyValueArgument width_option("width", 'w', &width);
  ray::OptionalKeyValueArgument max_trunk_radius_option("max_trunk_radius", 'm', &max_trunk_radius);
  ray::OptionalKeyValueArgument dimension_option("dimension", 'd', &dimension);
  ray::OptionalKeyValueArgument tree_density_option("tree_density", 't', &tree_density);
  ray::OptionalKeyValueArgument random_factor_option("random_factor", 'r', &random_factor);

  bool tree_parsed = ray::parseCommandLine(argc, argv, {&tree_text, &seed}, {&max_trunk_radius_option, &random_factor_option});
  bool forest_parsed = ray::parseCommandLine(argc, argv, {&forest_text, &seed}, {&width_option, &max_trunk_radius_option, &dimension_option, &tree_density_option, &random_factor_option});
  if (!tree_parsed && !forest_parsed)
  {
    usage();
  }
  ray::srand(seed.value());

  ray::ForestParams params;
  params.field_width = width_option.isSet() ? width.value() : 20;
  params.max_tree_radius = max_trunk_radius_option.isSet() ? max_trunk_radius.value() : 0.2; 
  params.dimension = dimension_option.isSet() ? dimension.value() : 2.0;
  params.adult_tree_density = tree_density_option.isSet() ? tree_density.value() : 0.01;
  params.random_factor = random_factor_option.isSet() ? random_factor.value() : 0.25;  

  ray::ForestGen forest;
  if (tree_parsed)
  {
    forest.trees.resize(1);
    ray::TreeStructure &tree = forest.trees[0];
    tree.segments().resize(1);
    tree.segments()[0].tip = Eigen::Vector3d(0, 0, 0);
    tree.segments()[0].radius = params.max_tree_radius;
    tree.make(params.random_factor);
    forest.save("tree.txt");
  }
  else
  {
    forest.make(params);
    forest.save("forest.txt");
  }
}


