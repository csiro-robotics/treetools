// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <raylib/rayforeststructure.h>
#include <raylib/raymesh.h>
#include <raylib/rayparse.h>
#include <raylib/rayply.h>
#include <cstdlib>
#include <iostream>
#include "treelib/treeutils.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Export the trees to a mesh, auto scaling any colour by default" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treemesh forest.txt" << std::endl;
  std::cout << "                    --max_colour 1 - specify the value that gives full brightness" << std::endl;
  std::cout << "                    --max_colour 1,0.1,1 - per-channel maximums (0 auto-scales to fit)" << std::endl;
  std::cout << "                    --rescale_colours - rescale each colour channel independently to fit in range" << std::endl;
  std::cout << "                    --view   - views the output immediately assuming meshlab is installed" << std::endl;
  std::cout << "                    --capsules - generate branch segments as the individual capsules" << std::endl;
  // clang-format on
  exit(exit_code);
}

// forward declarations
void addCapsule(ray::Mesh &mesh, const Eigen::Vector3d &pos1, const Eigen::Vector3d &pos2, double radius,
                ray::RGBA rgba);
void addCapsulePiece(ray::Mesh &mesh, int wind, const Eigen::Vector3d &pos, const Eigen::Vector3d &side1,
                     const Eigen::Vector3d &side2, double radius, const ray::RGBA &rgba, bool cap_start, bool cap_end);
void generateSmoothMesh(ray::Mesh &mesh, const ray::ForestStructure &forest, int red_id, double red_scale,
                        double green_scale, double blue_scale);

/// This method converts the tree file into a .ply mesh structure, with one cylinder approximation
/// per segment, coloured according to the tree file's colour attributes.
/// The -v option can be used if you have meshlab installed, to view the result immediately.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  ray::DoubleArgument max_brightness;
  ray::OptionalFlagArgument view("view", 'v'), capsules_option("capsules", 'c');
  ray::Vector3dArgument max_colour;
  ray::OptionalKeyValueArgument max_brightness_option("max_colour", 'm', &max_brightness);
  ray::OptionalKeyValueArgument max_colour_option("max_colour", 'm', &max_colour);

  const bool max_brightness_format =
    ray::parseCommandLine(argc, argv, { &forest_file }, { &max_brightness_option, &view, &capsules_option });
  const bool max_colour_format =
    ray::parseCommandLine(argc, argv, { &forest_file }, { &max_colour_option, &view, &capsules_option });
  if (!max_brightness_format && !max_colour_format)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }
  std::cout << "number of trees: " << forest.trees.size()
            << " num segments of first tree: " << forest.trees[0].segments().size() << std::endl;
  if (forest.trees[0].segments().size() == 1)
  {
    std::cerr << "Error: currently does not support exporting trunks only to a mesh" << std::endl;
    usage();
  }

  // if colouring the mesh:
  int red_id = -1;
  auto &att = forest.trees[0].attributeNames();
  const auto &it = std::find(att.begin(), att.end(), "red");
  if (it != att.end())
  {
    red_id = static_cast<int>(it - att.begin());
  }
  double red_scale = 1.0;
  double green_scale = 1.0;
  double blue_scale = 1.0;
  if (red_id != -1)
  {
    // option to rescale overall brightness
    if (max_brightness_option.isSet())
    {
      red_scale = green_scale = blue_scale = 255.0 / max_brightness.value();
    }
    // option to rescale each channel within a range
    else if (max_colour_option.isSet())
    {
      red_scale = 255.0 / max_colour.value()[0];
      green_scale = 255.0 / max_colour.value()[1];
      blue_scale = 255.0 / max_colour.value()[2];

      for (int i = 0; i < 3; i++)
      {
        if (max_colour.value()[i] <= 0.0)
        {
          double max_col = 0.0;
          for (auto &tree : forest.trees)
          {
            for (size_t s = 1; s < tree.segments().size(); s++)
            {
              auto &segment = tree.segments()[s];
              max_col = std::max(max_col, segment.attributes[red_id + i]);
            }
          }
          double &channel = i == 0 ? red_scale : (i == 1 ? green_scale : blue_scale);
          channel = 255.0 / max_col;
        }
      }
    }
    // otherwise auto-scale
    else
    {
      double max_col = 0.0;
      for (auto &tree : forest.trees)
      {
        for (size_t s = 1; s < tree.segments().size(); s++)
        {
          auto &segment = tree.segments()[s];
          for (int i = 0; i < 3; i++)
          {
            max_col = std::max(max_col, segment.attributes[red_id + i]);
          }
        }
      }
      red_scale = green_scale = blue_scale = 255.0 / max_col;
      std::cout << "auto re-scaling colour based on max colour value of " << max_col << std::endl;
    }
  }
  ray::Mesh mesh;
  // if rendering as individual capsules
  if (capsules_option.isSet())
  {
    for (auto &tree : forest.trees)
    {
      // for each segment
      for (size_t i = 1; i < tree.segments().size(); i++)
      {
        // generate a capsule to its parent tip position
        auto &segment = tree.segments()[i];
        ray::RGBA rgba;
        rgba.red = 127;
        rgba.green = 127;
        rgba.blue = 127;
        rgba.alpha = 255;
        if (red_id != -1)  // using per-segment colouring if supplied
        {
          rgba.red = uint8_t(std::min(red_scale * segment.attributes[red_id], 255.0));
          rgba.green = uint8_t(std::min(green_scale * segment.attributes[red_id + 1], 255.0));
          rgba.blue = uint8_t(std::min(blue_scale * segment.attributes[red_id + 2], 255.0));
        }
        addCapsule(mesh, segment.tip, tree.segments()[segment.parent_id].tip, segment.radius, rgba);
      }
    }
  }
  // otherwise use a smooth mesh generation function
  else
  {
    forest.generateSmoothMesh(mesh, red_id, red_scale, green_scale, blue_scale);
  }
  ray::writePlyMesh(forest_file.nameStub() + "_mesh.ply", mesh, true);
  // for convenience we can view the results immediately
  if (view.isSet())
  {
    return system(("meshlab " + forest_file.nameStub() + "_mesh.ply").c_str());
  }
  return 0;
}

/// @brief add the capsule (cylinder with hemispherical ends) to the mesh, approximated with 6 circumferential vertices
/// @param mesh the mesh to add the capsule to
/// @param pos1 base centre of capsule
/// @param pos2 end centre of capsule
/// @param radius radius of capsule
/// @param rgba colour of capsule
void addCapsule(ray::Mesh &mesh, const Eigen::Vector3d &pos1, const Eigen::Vector3d &pos2, double radius,
                ray::RGBA rgba)
{
  const int n = static_cast<int>(mesh.vertices().size());
  const Eigen::Vector3i N(n, n, n);
  std::vector<Eigen::Vector3i> &indices = mesh.indexList();
  std::vector<Eigen::Vector3d> vertices(14);

  const Eigen::Vector3d dir = (pos2 - pos1).normalized();
  const Eigen::Vector3d diag(1, 2, 3);
  const Eigen::Vector3d side1 = dir.cross(diag).normalized();
  const Eigen::Vector3d side2 = side1.cross(dir);

  vertices[12] = pos1 - radius * dir;
  vertices[13] = pos2 + radius * dir;
  for (int i = 0; i < 6; i++)
  {
    const double pi = 3.14156;
    const double angle = static_cast<double>(i) * 2.0 * pi / 6.0;
    const double angle2 = angle + pi / 6.0;
    vertices[i] = pos1 + radius * (side1 * std::sin(angle) + side2 * std::cos(angle));
    vertices[i + 6] = pos2 + radius * (side1 * std::sin(angle2) + side2 * std::cos(angle2));
    indices.push_back(N + Eigen::Vector3i(12, i, (i + 1) % 6));  // start end
    indices.push_back(N + Eigen::Vector3i(13, (i + 1) % 6 + 6, i + 6));
    indices.push_back(N + Eigen::Vector3i(i, i + 6, (i + 1) % 6));
    indices.push_back(N + Eigen::Vector3i((i + 1) % 6 + 6, (i + 1) % 6, i + 6));
  }
  mesh.vertices().insert(mesh.vertices().end(), vertices.begin(), vertices.end());
  for (size_t i = 0; i < vertices.size(); i++)
  {
    mesh.colours().push_back(rgba);
  }
}
