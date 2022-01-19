// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#include <raylib/rayforestgen.h>
#include <raylib/raytreegen.h>
#include <raylib/rayparse.h>
#include <raylib/raycloud.h>
#include <raylib/rayrenderer.h>
#define STB_IMAGE_IMPLEMENTATION
#include "raylib/imageread.h"
#include <cstdlib>
#include <iostream>

/// Function to colour the cloud from a horizontal projection of a supplied image, stretching to match the cloud bounds.
void colourFromImage(const std::string &cloud_file, const std::string &image_file, ray::CloudWriter &writer)
{
  ray::Cloud::Info info;
  if (!ray::Cloud::getInfo(cloud_file, info))
  {
    usage();
  }
  const ray::Cuboid bounds = info.ends_bound;
  stbi_set_flip_vertically_on_load(1);
  int width, height, num_channels;
  unsigned char *image_data = stbi_load(image_file.c_str(), &width, &height, &num_channels, 0);
  const double width_x = (bounds.max_bound_[0] - bounds.min_bound_[0])/(double)width;
  const double width_y = (bounds.max_bound_[1] - bounds.min_bound_[1])/(double)height;
  if (std::max(width_x, width_y) > 1.05 * std::min(width_x, width_y))
  {
    std::cout << "Warning: image aspect ratio does not match aspect match of point cloud (" << 
      bounds.max_bound_[0] - bounds.min_bound_[0] << " x " << bounds.max_bound_[1] - bounds.min_bound_[1] << ", stretching to fit) " << std::endl;
  }

  auto colour_from_image = [&bounds, &writer, &image_data, width_x, width_y, width, height, num_channels]
    (std::vector<Eigen::Vector3d> &starts, std::vector<Eigen::Vector3d> &ends, 
    std::vector<double> &times, std::vector<ray::RGBA> &colours)
  {
    for (size_t i = 0; i<ends.size(); i++)
    {
      const int ind0 = static_cast<int>((ends[i][0] - bounds.min_bound_[0]) / width_x);
      const int ind1 = static_cast<int>((ends[i][1] - bounds.min_bound_[1]) / width_y);
      const int index = num_channels*(ind0 + width*ind1);
      colours[i].red = image_data[index];
      colours[i].green = image_data[index+1];
      colours[i].blue = image_data[index+2];
    }
    writer.writeChunk(starts, ends, times, colours);
  };

  if (!ray::Cloud::read(cloud_file, colour_from_image))
    usage();

  stbi_image_free(image_data);
}


void usage(int exit_code = 1)
{
  std::cout << "Add information to a tree file" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treeadd forest.txt LAI_image.hdr 10.3,-12.4,0.2 - applies image colour to the tree file" << std::endl;
  std::cout << "                                                  at min coordinate 10.3,-12.4 and pixel width 0.2m" << std::endl;
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

  ray::ForestGen forest;
  ray::ForestParams params;
  if (!forest.makeFromFile(forest_file.name(), params))
  {
    usage();
  }  
  std::vector<ray::TreeGen> &trees = forest.trees();

  std::cout << "reading image: " << image_file.name() << std::endl;
  std::string ext = image_file.nameExt();

  const char *image_name = image_file.name().c_str();
  stbi_set_flip_vertically_on_load(1);
  int width, height, num_channels;
  unsigned char *image_data = stbi_load(image_name, &width, &height, &num_channels, 0);

  for (auto &tree: trees)
  {
    Eigen::Vector2d centre(tree.root()[0], tree.root()[1]);
    double rad = tree.tree_radius;
    Eigen::Vector2d minpos = centre - Eigen::Vector2d(rad, rad);
    Eigen::Vector2d maxpos = centre + Eigen::Vector2d(rad, rad);
    Eigen::Vector2d coords(coord.value()[0], coord.value[1]);
    Eigen::Vector2i boxmin = (minpos - coord.value()) / coord.value()[2]).cast<int>();
    Eigen::Vector2i boxmax = (maxpos - coord.value()) / coord.value()[2]).cast<int>();
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
        colour[0] += (double)image_data[index];
        colour[1] += (double)image_data[index+1];
        colour[2] += (double)image_data[index+2];      
        count++;  
      }
    }
    if (count > 0)
      colour /= (double)count;
    tree.colour = colour;
  }

  // so this is basically it, but we need to add:
  // 1. treeGen structure needs a tree_radius, it needs to be read in
  // 2. we need a treegenerate function to turn the tree into a mesh (and colour by the colour)
  // 3. we need it to work on hdr
  // 4. we need to save out the tree, including the tree_radius value
}


