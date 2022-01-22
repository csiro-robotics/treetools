// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "treelib/imageread.h"
#include <raylib/rayforestgen.h>
#include <raylib/raytreegen.h>
#include <raylib/rayparse.h>
#include <raylib/raycloud.h>
#include <raylib/rayrenderer.h>
#include "raylib/raytreegen.h"
#include <cstdlib>
#include <iostream>

void usage(int exit_code = 1)
{
  std::cout << "Colour a tree file from an image" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treecolour forest.txt LAI_image.hdr 10.3,-12.4,0.2 - applies image colour to the tree file" << std::endl;
  std::cout << "                                                     at min coordinate 10.3,-12.4 and pixel width 0.2m" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, image_file;
  ray::Vector3dArgument coord;
  bool parsed = ray::parseCommandLine(argc, argv, {&forest_file, &image_file, &coord});
  if (!parsed)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }  

  std::cout << "reading image: " << image_file.name() << std::endl;
  std::string ext = image_file.nameExt();

  const char *image_name = image_file.name().c_str();
  stbi_set_flip_vertically_on_load(1);
  int width, height, num_channels;
  unsigned char *image_data = 0;
  float *image_dataf = 0;
  bool is_hdr = image_file.nameExt() == "hdr";
  if (is_hdr)
    image_dataf = stbi_loadf(image_name, &width, &height, &num_channels, 0);
  else 
    image_data = stbi_load(image_name, &width, &height, &num_channels, 0);

  int tree_radius_id = -1;
  for (size_t i = 0; i<forest.trees[0].attributes().size(); i++)
    if (forest.trees[0].attributes()[i] == "tree_radius")
      tree_radius_id = i;
  double trunk_to_tree_radius_scale = 10.0;
  if (tree_radius_id == -1)
    std::cout << "Warning: tree file does not contain tree radii, so they are estimated as " << trunk_to_tree_radius_scale << " times the trunk radius." << std::endl;

  for (auto &tree: forest.trees)
  {
    tree.attributes().push_back("red");
    tree.attributes().push_back("green");
    tree.attributes().push_back("blue");
    Eigen::Vector2d centre(tree.root()[0], tree.root()[1]);
    double rad = tree_radius_id == -1 ? tree.segments()[0].radius * trunk_to_tree_radius_scale : tree.segments()[0].attributes[tree_radius_id];
    Eigen::Vector2d minpos = centre - Eigen::Vector2d(rad, rad);
    Eigen::Vector2d maxpos = centre + Eigen::Vector2d(rad, rad);
    Eigen::Vector2d coords(coord.value()[0], coord.value()[1]);
    Eigen::Vector2i boxmin = ((minpos - coords) / coord.value()[2]).cast<int>();
    Eigen::Vector2i boxmax = ((maxpos - coords) / coord.value()[2]).cast<int>();
    Eigen::Vector3d colour(0,0,0);
    int count = 0;
    for (int i = boxmin[0]; i<=boxmax[0]; i++)
    {
      for (int j = boxmin[1]; j<=boxmax[1]; j++)
      {
        Eigen::Vector2d pos = Eigen::Vector2d(i,j)*coord.value()[2] + coords;
        if ((pos - centre).norm() > rad)
          continue;
        const int index = num_channels*(i + width*j);
        if (is_hdr)
        {
          colour[0] += (double)image_dataf[index];
          colour[1] += (double)image_dataf[index+1];
          colour[2] += (double)image_dataf[index+2];      
        }
        else
        {
          colour[0] += (double)image_data[index];
          colour[1] += (double)image_data[index+1];
          colour[2] += (double)image_data[index+2];      
        }
        count++;  
      }
    }
    if (count > 0)
      colour /= (double)count;
    tree.segments()[0].attributes.push_back(colour[0]);
    tree.segments()[0].attributes.push_back(colour[1]);
    tree.segments()[0].attributes.push_back(colour[2]);
  }
  forest.save(forest_file.nameStub() + "_coloured.txt");
}


