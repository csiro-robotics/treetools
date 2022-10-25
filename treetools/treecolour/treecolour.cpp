// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#define STB_IMAGE_IMPLEMENTATION
#include <raylib/raycloud.h>
#include <raylib/rayforeststructure.h>
#include <raylib/rayparse.h>
#include <raylib/rayrenderer.h>
#include <raylib/raytreegen.h>
#include <cstdlib>
#include <iostream>
#include "raylib/raytreegen.h"
#include "treelib/imageread.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Colour a tree file from an image" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treecolour forest.txt diameter - greyscale by branch diameter, or any other attribute" << std::endl;
  std::cout << "                      trunk diameter - greyscale by diameter (or any other attribute) for the trunk" << std::endl;
  std::cout << "                      tree height - greyscale by tree height (or any other attribute) for the whole tree" << std::endl;
  std::cout << std::endl;
  std::cout << "                      1,length,diameter - rgb values or per-segment attributes" << std::endl;
  std::cout << "                      trunk 1,length,diameter - rgb values or per-trunk attributes" << std::endl;
  std::cout << "                      tree 1,height,width - rgb values or per-tree attributes" << std::endl;
  std::cout << std::endl;
  std::cout << "                      LAI_image.hdr 10.3,-12.4,0.2 - applies image colour to the tree file" << std::endl;
  std::cout << "                                                     at min coordinate 10.3,-12.4 and pixel width 0.2m" << std::endl;
  std::cout << "                    --multiplier 1  - apply colour scale" << std::endl;
  std::cout << "                    --scale 1,0.1,1 - apply per-channel scales" << std::endl;
  std::cout << "                    --gradient_rgb - apply a red->green->blue gradient instead of greyscale" << std::endl;
  // clang-format on
  exit(exit_code);
}

// This method adds a red, green, blue component to the tree file, set based on the specified
// colouration scheme, such as according to the value of a specified attribute in the file.
// By default treecolour colours in shades of grey, unless you specify the gradient_rgb.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, image_file, attribute(false);
  ray::TextArgument trunk("trunk"), whole_tree("tree");
  ray::DoubleArgument scale;
  ray::Vector3dArgument coord, scale3D;
  ray::OptionalFlagArgument gradient_rgb("gradient_rgb", 'g');
  ray::OptionalKeyValueArgument scale3D_option("scale", 's', &scale3D);
  ray::OptionalKeyValueArgument scale_option("multiplier", 'm', &scale);
  const bool attribute_format =
    ray::parseCommandLine(argc, argv, { &forest_file, &attribute }, { &scale3D_option, &scale_option, &gradient_rgb });
  const bool trunk_attribute_format = ray::parseCommandLine(argc, argv, { &forest_file, &trunk, &attribute },
                                                      { &scale3D_option, &scale_option, &gradient_rgb });
  const bool  tree_attribute_format = ray::parseCommandLine(argc, argv, { &forest_file, &whole_tree, &attribute },
                                                      { &scale3D_option, &scale_option, &gradient_rgb });
  const bool image_format =
    ray::parseCommandLine(argc, argv, { &forest_file, &image_file, &coord }, { &scale3D_option, &scale_option });
  if (!image_format && !attribute_format && !trunk_attribute_format && !tree_attribute_format)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }

  auto &input_attributes = tree_attribute_format ? forest.trees[0].treeAttributeNames() : forest.trees[0].attributeNames();
  auto &att = forest.trees[0].attributeNames();
  int red_id = (int)att.size();
  const auto &it = std::find(att.begin(), att.end(), "red");
  if (it != att.end())
  {
    red_id = (int)(it - att.begin());  // we always assume that red is followed immediately by attributes green and blue
    std::cout << "colour attributes found, so replacing these in the output file" << std::endl;
  }
  else  // no colour found so lets add empty values across the whole structure
  {
    for (auto &tree : forest.trees)
    {
      tree.attributeNames().push_back("red");
      tree.attributeNames().push_back("green");
      tree.attributeNames().push_back("blue");
      for (auto &segment : tree.segments())
      {
        segment.attributes.push_back(0);
        segment.attributes.push_back(0);
        segment.attributes.push_back(0);
      }
    }
  }
  int attribute_ids[3] = { -1, -1, -1 };
  Eigen::Vector3d colour(0, 0, 0);
  int num_attributes = 0;
  if (attribute_format || trunk_attribute_format || tree_attribute_format)
  {
    std::stringstream ss(attribute.name());
    std::string field;
    while (std::getline(ss, field, ','))
    {
      if (num_attributes == 3)
      {
        std::cerr << "error: bad format for r,g,b: " << attribute.name() << std::endl;
        usage();
      }
      char *endptr;
      const char *str = field.c_str();
      colour[num_attributes] = std::strtod(str, &endptr);
      if (endptr != str + std::strlen(str))  // if conversion to a double fails then it must be an attribute
      {
        const auto &it = std::find(input_attributes.begin(), input_attributes.end(), field);
        if (it != input_attributes.end())
        {
          attribute_ids[num_attributes] = (int)(it - input_attributes.begin());
          std::cout << "found attribute " << field << " at index " << attribute_ids[num_attributes] << std::endl;
        }
        else
        {
          std::cerr << "Error: cannot find attribute " << field << " in the format of file " << forest_file.name()
                    << std::endl;
          usage();
        }
      }
      num_attributes++;
    }
    if (num_attributes == 1)
    {
      attribute_ids[2] = attribute_ids[1] = attribute_ids[0];
    }
    else if (num_attributes != 3)
    {
      std::cerr << "error: bad format for r,g,b: " << attribute.name() << std::endl;
      usage();
    }
  }
  if (scale_option.isSet())
  {
    std::cout << "linear scale set to " << scale.value() << std::endl;
  }
  else if (scale3D_option.isSet())
  {
    std::cout << "Per-channel scale option set to " << scale3D.value().transpose() << std::endl;
  }
  Eigen::Vector3d scalevec(1, 1, 1);
  if (scale_option.isSet())
  {
    scalevec = Eigen::Vector3d(scale.value(), scale.value(), scale.value());
  }
  else if (scale3D_option.isSet())
  {
    scalevec = scale3D.value();
  }

  if (attribute_format)
  {
    for (auto &tree : forest.trees)
    {
      for (auto &segment : tree.segments())
      {
        for (int i = 0; i < 3; i++)
        {
          segment.attributes[red_id + i] =
            (attribute_ids[i] == -1 ? colour[i] : segment.attributes[attribute_ids[i]]) * scalevec[i];
          if (num_attributes == 1 && gradient_rgb.isSet())  // then apply the gradient
          {
            segment.attributes[red_id + i] = ray::redGreenBlueGradient(segment.attributes[red_id + i])[i];
          }
        }
      }
    }
  }
  else if (trunk_attribute_format)
  {
    for (auto &tree : forest.trees)
    {
      Eigen::Vector3d col;
      for (int i = 0; i < 3; i++)
      {
        col[i] = attribute_ids[i] == -1 ? colour[i] : tree.segments()[0].attributes[attribute_ids[i]];
      }
      for (auto &segment : tree.segments())
      {
        segment.attributes[red_id + 0] = col[0] * scalevec[0];
        segment.attributes[red_id + 1] = col[1] * scalevec[1];
        segment.attributes[red_id + 2] = col[2] * scalevec[2];
      }
    }
  }
  else if (tree_attribute_format)
  {
    for (auto &tree : forest.trees)
    {
      Eigen::Vector3d col;
      for (int i = 0; i < 3; i++)
      {
        col[i] = attribute_ids[i] == -1 ? colour[i] : tree.treeAttributes()[attribute_ids[i]];
      }
      for (auto &segment : tree.segments())
      {
        segment.attributes[red_id + 0] = col[0] * scalevec[0];
        segment.attributes[red_id + 1] = col[1] * scalevec[1];
        segment.attributes[red_id + 2] = col[2] * scalevec[2];
      }
    }
  }
  else
  {
    std::cout << "reading image: " << image_file.name() << std::endl;
    std::string ext = image_file.nameExt();

    const char *image_name = image_file.name().c_str();
    stbi_set_flip_vertically_on_load(1);
    int width, height, num_channels;
    unsigned char *image_data = 0;
    float *image_dataf = 0;
    const bool is_hdr = image_file.nameExt() == "hdr";
    if (is_hdr)
    {
      image_dataf = stbi_loadf(image_name, &width, &height, &num_channels, 0);
    }
    else
    {
      image_data = stbi_load(image_name, &width, &height, &num_channels, 0);
    }

    int tree_radius_id = -1;
    for (size_t i = 0; i < forest.trees[0].attributeNames().size(); i++)
    {
      if (forest.trees[0].attributeNames()[i] == "subtree_radius")
      {
        tree_radius_id = (int)i;
      }
    }
    const double trunk_to_tree_radius_scale = 10.0;
    const bool trunks_only = forest.trees[0].segments().size() == 1;
    if (tree_radius_id == -1 && trunks_only)
    {
      std::cout << "Warning: tree file does not contain tree radii, so they are estimated as "
                << trunk_to_tree_radius_scale << " times the trunk radius." << std::endl;
    }

    for (auto &tree : forest.trees)
    {
      Eigen::Vector2d centre(tree.root()[0], tree.root()[1]);
      double rad = tree_radius_id == -1 ? tree.segments()[0].radius * trunk_to_tree_radius_scale :
                                          tree.segments()[0].attributes[tree_radius_id];
      // get the radius:
      if (tree_radius_id == -1 && trunks_only == false)  // no tree radius specified, so estimate it from the branches
      {
        const double big = 1e10;
        Eigen::Vector3d minbound(big, big, big), maxbound(-big, -big, -big);
        for (auto &segment : tree.segments())
        {
          minbound = ray::minVector(minbound, segment.tip);
          maxbound = ray::maxVector(maxbound, segment.tip);
        }
        const Eigen::Vector3d extent = (maxbound - minbound) / 2.0;
        rad = 0.5 * (extent[0] + extent[1]);  // the mean diameter of the points
      }

      const Eigen::Vector2d minpos = centre - Eigen::Vector2d(rad, rad);
      const Eigen::Vector2d maxpos = centre + Eigen::Vector2d(rad, rad);
      const Eigen::Vector2d coords(coord.value()[0], coord.value()[1]);
      const Eigen::Vector2i boxmin = ((minpos - coords) / coord.value()[2]).cast<int>();
      const Eigen::Vector2i boxmax = ((maxpos - coords) / coord.value()[2]).cast<int>();
      Eigen::Vector3d colour(0, 0, 0);
      int count = 0;
      for (int i = std::max(0, boxmin[0]); i <= std::min(boxmax[0], width - 1); i++)
      {
        for (int j = std::max(0, boxmin[1]); j <= std::min(boxmax[1], height - 1); j++)
        {
          Eigen::Vector2d pos = Eigen::Vector2d(i + 0.5, j + 0.5) * coord.value()[2] + coords;
          if ((pos - centre).norm() > rad)
          {
            continue;
          }
          const int index = num_channels * (i + width * j);
          if (is_hdr)
          {
            if (image_dataf[index] > 0.f || image_dataf[index + 1] > 0.f || image_dataf[index + 2] > 0.f)
            {
              colour[0] += (double)image_dataf[index];
              colour[1] += (double)image_dataf[index + 1];
              colour[2] += (double)image_dataf[index + 2];
              count++;
            }
          }
          else
          {
            if (image_data[index] > 0 || image_data[index + 1] > 0 || image_data[index + 2] > 0)
            {
              colour[0] += (double)image_data[index];
              colour[1] += (double)image_data[index + 1];
              colour[2] += (double)image_data[index + 2];
              count++;
            }
          }
        }
      }
      if (count > 0)
      {
        colour /= (double)count;
      }
      for (auto &segment : tree.segments())
      {
        segment.attributes[red_id + 0] = colour[0] * scalevec[0];
        segment.attributes[red_id + 1] = colour[1] * scalevec[1];
        segment.attributes[red_id + 2] = colour[2] * scalevec[2];
      }
    }
  }
  forest.save(forest_file.nameStub() + "_coloured.txt");
  return 0;
}
