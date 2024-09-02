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
  std::cout << "                    --uvs - generate uvs and points to a wood_texture.png which needs to be created. Works in CloudCompare, not Meshlab." << std::endl;
  std::cout << "                    --capsules  - generate branch segments as the individual capsules" << std::endl;
  std::cout << "                    --cylinders - generate branch segments as the individual cylinders" << std::endl;
  // clang-format on
  exit(exit_code);
}

// forward declarations
void addCapsule(ray::Mesh &mesh, const Eigen::Vector3d &pos1, const Eigen::Vector3d &pos2, double radius,
                ray::RGBA rgba, double cap_scale);
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
  ray::OptionalFlagArgument view("view", 'v'), capsules_option("capsules", 'c'), cylinders_option("cylinders", 'y'), uvs_option("uvs", 'u');
  ray::Vector3dArgument max_colour;
  ray::OptionalKeyValueArgument max_brightness_option("max_colour", 'm', &max_brightness);
  ray::OptionalKeyValueArgument max_colour_option("max_colour", 'm', &max_colour);

  const bool max_brightness_format =
    ray::parseCommandLine(argc, argv, { &forest_file }, { &max_brightness_option, &view, &capsules_option, &cylinders_option, &uvs_option });
  const bool max_colour_format =
    ray::parseCommandLine(argc, argv, { &forest_file }, { &max_colour_option, &view, &capsules_option, &cylinders_option, &uvs_option });
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
  if (capsules_option.isSet() || cylinders_option.isSet())
  {
    double cap_scale = capsules_option.isSet() ? 1.0 : 0.0;
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
        addCapsule(mesh, segment.tip, tree.segments()[segment.parent_id].tip, segment.radius, rgba, cap_scale);
      }
    }
  }
  // otherwise use a smooth mesh generation function
  else
  {
    forest.generateSmoothMesh(mesh, red_id, red_scale, green_scale, blue_scale, uvs_option.isSet());
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
                ray::RGBA rgba, double cap_scale)
{
  const int n = static_cast<int>(mesh.vertices().size());
  const Eigen::Vector3i N(n, n, n);
  std::vector<Eigen::Vector3i> &indices = mesh.indexList();
  std::vector<Eigen::Vector3d> vertices(14);

  const Eigen::Vector3d dir = (pos2 - pos1).normalized();
  const Eigen::Vector3d diag(1, 2, 3);
  const Eigen::Vector3d side1 = dir.cross(diag).normalized();
  const Eigen::Vector3d side2 = side1.cross(dir);

  vertices[12] = pos1 - radius * dir * cap_scale;
  vertices[13] = pos2 + radius * dir * cap_scale;
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

// add a single section of a capsule. Each one is like a node in the polyline with a radius.
void addCapsulePiece(ray::Mesh &mesh, int wind, const Eigen::Vector3d &pos, const Eigen::Vector3d &side1,
                     const Eigen::Vector3d &side2, double radius, const ray::RGBA &rgba, bool cap_start, bool cap_end)
{
  const int start_index = static_cast<int>(mesh.vertices().size());
  const Eigen::Vector3i start_indices(start_index, start_index, start_index);  // start indices
  std::vector<Eigen::Vector3i> &indices = mesh.indexList();
  std::vector<Eigen::Vector3d> vertices;
  vertices.reserve(7);
  Eigen::Vector3d dir = side2.cross(side1);
  if (cap_start)
    vertices.push_back(pos - radius * dir);

  // add the six vertices in the circumferential ring for this point along the branch
  for (int i = 0; i < 6; i++)
  {
    const double pi = 3.14156;
    double angle = (static_cast<double>(i) * 2.0 + static_cast<double>(wind)) * pi / 6.0;

    vertices.push_back(pos + radius * (side1 * std::sin(angle) + side2 * std::cos(angle)));
    // the indexing is a bit more complicated, to connect the vertices with triangles
    if (cap_start)
    {
      indices.push_back(start_indices + Eigen::Vector3i(0, 1 + i, 1 + ((i + 1) % 6)));
    }
    else
    {
      indices.push_back(start_indices + Eigen::Vector3i(i - 6, i, ((i + 1) % 6) - 6));
      indices.push_back(start_indices + Eigen::Vector3i((i + 1) % 6, ((i + 1) % 6) - 6, i));
    }
  }
  if (cap_end)
  {
    vertices.push_back(pos + radius * dir);
    for (int i = 0; i < 6; i++)
    {
      indices.push_back(start_indices + Eigen::Vector3i(6, (i + 1) % 6, i));
    }
  }
  // add these vertices into the mesh
  mesh.vertices().insert(mesh.vertices().end(), vertices.begin(), vertices.end());
  for (size_t i = 0; i < vertices.size(); i++)
  {
    mesh.colours().push_back(rgba);
  }
}

/// @brief This converts the piecewise cylindrical model into a smoother mesh than individual capsule meshes
///        Specifically, each branch (from its base up through the widest radius at each bifurcation) is a continuous
///        mesh with 6 vertices around its circumference. This is equivalent to the capsules being connected
///        wherever it is a continuation of the branch. The result is fewer triangles and a smoother result.
/// @param mesh the mesh object to generate into
/// @param forest the forest structure representing the piecewise cylindrical trees
/// @param red_id the first colour channel id, used to colour the trees
/// @param red_scale scale on the red colour component
/// @param green_scale scale on the green channel
/// @param blue_scale scale on the blue channel
void generateSmoothMesh(ray::Mesh &mesh, const ray::ForestStructure &forest, int red_id, double red_scale,
                        double green_scale, double blue_scale)
{
  for (const auto &tree : forest.trees)
  {
    const auto &segments = tree.segments();
    // first generate the list of children for each segment
    std::vector<std::vector<int>> children(segments.size());
    for (size_t i = 0; i < segments.size(); i++)
    {
      const auto &segment = segments[i];
      int parent = segment.parent_id;
      if (parent != -1)
      {
        children[parent].push_back((int)i);
      }
    }
    // now generate the set of root segments
    std::vector<int> roots;
    for (int i = 1; i < static_cast<int>(segments.size()); i++)
    {
      if (segments[i].parent_id > 0)
      {
        break;
      }
      roots.push_back(i);
    }

    ray::RGBA rgba;
    // for each root, we follow up through the largest child to make a contiguous branch
    for (size_t i = 0; i < roots.size(); i++)
    {
      int root_id = roots[i];
      Eigen::Vector3d normal(1, 2, 3);  // unspecial 'up' direction for placing vertices along the circumference

      // we iterate through this list and grow it at the same time
      std::vector<int> childlist = { root_id };
      int wind =
        0;  // this is what rotates the vertices half a triangle width at each segment, to keep the triangles isoceles
      for (size_t j = 0; j < childlist.size(); j++)
      {
        int child_id = childlist[j];
        int par_id = segments[child_id].parent_id;
        // generate an orthogonal frame for each ring of vertices to sit on
        Eigen::Vector3d dir = (segments[child_id].tip - segments[par_id].tip).normalized();
        Eigen::Vector3d axis1 = normal.cross(dir).normalized();
        Eigen::Vector3d axis2 = axis1.cross(dir);
        rgba = ray::RGBA::treetrunk();  // standardised colour in raycloudtools
        if (red_id != -1)               // use the per-segment colour if it exists (e.g. from treecolour)
        {
          rgba.red = uint8_t(std::min(red_scale * segments[child_id].attributes[red_id], 255.0));
          rgba.green = uint8_t(std::min(green_scale * segments[child_id].attributes[red_id + 1], 255.0));
          rgba.blue = uint8_t(std::min(blue_scale * segments[child_id].attributes[red_id + 2], 255.0));
        }

        if (child_id == root_id)  // add the base cap of the cylinder if we are at the root of the branch
        {
          addCapsulePiece(mesh, wind, segments[par_id].tip, axis1, axis2, segments[child_id].radius, rgba, true, false);
        }

        wind++;
        std::vector<int> kids = children[child_id];
        if (kids.empty())  // add the end cap of the cylinder if we are at the end of the whole branch
        {
          addCapsulePiece(mesh, wind, segments[child_id].tip, axis1, axis2, segments[child_id].radius, rgba, false,
                          true);
          break;
        }
        // now find the maximum radius subbranch
        double max_rad = 0.0;
        int max_k = 0;
        for (int k = 0; k < static_cast<int>(kids.size()); k++)
        {
          double rad = segments[kids[k]].radius;
          if (rad > max_rad)
          {
            max_rad = rad;
            max_k = k;
          }
        }
        for (int k = 0; k < static_cast<int>(kids.size()); k++)
        {
          if (k != max_k)
          {
            roots.push_back(kids[k]);  // all other subbranches get added to the list, to be iterated over on their turn
          }
        }

        int next_id = kids[max_k];
        Eigen::Vector3d dir2 = (segments[next_id].tip - segments[child_id].tip).normalized();

        Eigen::Vector3d top_dir = (dir2 + dir).normalized();  // here we average the directions of the two segments
        // and generate an orthogonal basis for the ring of points on the branch
        Eigen::Vector3d mid_axis1 = normal.cross(top_dir).normalized();
        Eigen::Vector3d mid_axis2 = mid_axis1.cross(top_dir);
        normal = -mid_axis2;
        // add the ring of points
        addCapsulePiece(mesh, wind, segments[child_id].tip, mid_axis1, mid_axis2, segments[child_id].radius, rgba,
                        false, false);
        // add the biggest subbranch to the list, so we continue to build the branch
        childlist.push_back(kids[max_k]);
      }
    }
  }
}
