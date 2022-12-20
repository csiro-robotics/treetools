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
  std::cout << "                    --smooth - makes each main branch a continuous mesh" << std::endl;
  // clang-format on
  exit(exit_code);
}

// add a cylinder to the @c mesh
void addCylinder(ray::Mesh &mesh, const Eigen::Vector3d &pos1, const Eigen::Vector3d &pos2, double radius,
                 ray::RGBA rgba)
{
  const int n = (int)mesh.vertices().size();
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
    const double angle = (double)i * 2.0 * pi / 6.0;
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

void addOpenCylinder(ray::Mesh &mesh, const Eigen::Vector3d &pos1, const Eigen::Vector3d &pos2, 
                 double radius, double top_radius,
                 const Eigen::Vector3d &side1, const Eigen::Vector3d &side2, 
                 const Eigen::Vector3d &top_side1, const Eigen::Vector3d &top_side2, ray::RGBA rgba,
                 bool cap_start, bool cap_end)
{
  const int n = (int)mesh.vertices().size();
  const Eigen::Vector3i N(n, n, n);
  std::vector<Eigen::Vector3i> &indices = mesh.indexList();
  int num_verts = 12;
  if (cap_start)
    num_verts++;
  if (cap_end)
    num_verts++;
  std::vector<Eigen::Vector3d> vertices(num_verts);
  Eigen::Vector3d dir = (pos2 - pos1).normalized();
  int last = 12;
  if (cap_start)
    vertices[last++] = pos1 - radius * dir;
  if (cap_end)
    vertices[last++] = pos2 + top_radius * dir;

  for (int i = 0; i < 6; i++)
  {
    const double pi = 3.14156;
    const double angle = (double)i * 2.0 * pi / 6.0;
    const double angle2 = angle + pi / 6.0;
    vertices[i] = pos1 + radius * (side1 * std::sin(angle) + side2 * std::cos(angle));
    vertices[i + 6] = pos2 + top_radius * (top_side1 * std::sin(angle2) + top_side2 * std::cos(angle2));
    indices.push_back(N + Eigen::Vector3i(i, i + 6, (i + 1) % 6));
    indices.push_back(N + Eigen::Vector3i((i + 1) % 6 + 6, (i + 1) % 6, i + 6));

    int last = 12;
    if (cap_start)
      indices.push_back(N + Eigen::Vector3i(last++, i, (i + 1) % 6));  
    if (cap_end)
      indices.push_back(N + Eigen::Vector3i(last++, (i + 1) % 6 + 6, i + 6));
  }
  mesh.vertices().insert(mesh.vertices().end(), vertices.begin(), vertices.end());
  for (size_t i = 0; i < vertices.size(); i++)
  {
    mesh.colours().push_back(rgba);
  }
}


void generateSmoothMesh(ray::Mesh &mesh, const ray::ForestStructure &forest)
{
  for (const auto &tree : forest.trees)
  {
    const auto &segments = tree.segments();
    std::vector<std::vector<int> > children(segments.size());
    for (size_t i = 0; i < segments.size(); i++)
    {
      const auto &segment = segments[i];
      int parent = segment.parent_id;
      if (parent != -1)
      {
        children[parent].push_back(i);
      }
    }
    std::vector<int> roots;
    for (size_t i = 1; i < segments.size(); i++)
    {
      if (segments[i].parent_id > 0)
      {
        break;
      }
      roots.push_back(i);
    }
    ray::RGBA rgba(127, 127, 127, 255);
    // for each root
    for (size_t i = 0; i < roots.size(); i++)
    {
      int root_id = roots[i];
      Eigen::Vector3d normal(1,2,3); // unlikely that any cylinder directions will exactly match this
      Eigen::Vector3d last_dir = (segments[root_id].tip - segments[segments[root_id].parent_id].tip).normalized();
   //   Eigen::Vector3d last_axis1 = normal.cross(last_dir).normalized();
   //   Eigen::Vector3d last_axis2 = last_axis1.cross(last_dir);

      std::vector<int> childlist = {root_id};
      for (size_t j = 0; j<childlist.size(); j++)
      {
        int child_id = childlist[j];
        int par_id = segments[child_id].parent_id;
        Eigen::Vector3d dir = (segments[child_id].tip - segments[par_id].tip).normalized();
        Eigen::Vector3d last_axis1 = normal.cross(last_dir).normalized();
        Eigen::Vector3d last_axis2 = last_axis1.cross(last_dir);
        
        double parent_rad = (child_id != root_id) ? segments[par_id].radius : segments[child_id].radius;

        std::vector<int> kids = children[child_id];
        if (kids.empty())
        {
          Eigen::Vector3d axis1 = normal.cross(dir).normalized();
          Eigen::Vector3d axis2 = last_axis1.cross(dir);
          addOpenCylinder(mesh, segments[par_id].tip, segments[child_id].tip, parent_rad, segments[child_id].radius, 
            last_axis1, last_axis2, axis1, axis2, rgba, child_id==root_id, true);
          break;
        }
        double max_rad = 0.0;
        int max_k = 0;
        for (size_t k = 0; k<kids.size(); k++)
        {
          double rad = segments[kids[k]].radius;
          if (rad > max_rad)
          {
            max_rad = rad;
            max_k = k;
          }
        }
        for (size_t k = 0; k<kids.size(); k++)
        {
          if (k != max_k)
          {
            roots.push_back(kids[k]);
          }
        }

        int next_id = kids[max_k];
        Eigen::Vector3d dir2 = (segments[next_id].tip - segments[child_id].tip).normalized();
        
        Eigen::Vector3d top_dir = (dir2 + dir).normalized();
        Eigen::Vector3d axis1 = normal.cross(top_dir).normalized();
        Eigen::Vector3d axis2 = last_axis1.cross(top_dir);
        normal = axis1;

        addOpenCylinder(mesh, segments[par_id].tip, segments[child_id].tip, parent_rad, segments[child_id].radius, 
          last_axis1, last_axis2, axis1, axis2, rgba, child_id==root_id, false);
        
        last_dir = top_dir;

        childlist.push_back(kids[max_k]);
      }
    }
  }
}


/// This method converts the tree file into a .ply mesh structure, with one cylinder approximation 
/// per segment, coloured according to the tree file's colour attributes. 
/// The -v option can be used if you have meshlab installed, to view the result immediately.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  ray::DoubleArgument max_brightness;
  ray::OptionalFlagArgument view("view", 'v'), smooth_option("smooth", 's');
  ray::Vector3dArgument max_colour;
  ray::OptionalKeyValueArgument max_brightness_option("max_colour", 'm', &max_brightness);
  ray::OptionalKeyValueArgument max_colour_option("max_colour", 'm', &max_colour);

  const bool max_brightness_format =
    ray::parseCommandLine(argc, argv, { &forest_file }, { &max_brightness_option, &view });
  const bool max_colour_format = ray::parseCommandLine(argc, argv, { &forest_file }, { &max_colour_option, &view, &smooth_option });
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
    red_id = (int)(it - att.begin());
  }
  double red_scale = 1.0;
  double green_scale = 1.0;
  double blue_scale = 1.0;
  if (red_id != -1)
  {
    if (max_brightness_option.isSet())
    {
      red_scale = green_scale = blue_scale = 255.0 / max_brightness.value();
    }
    else if (max_colour_option.isSet())
    {
      red_scale = 255.0 / max_colour.value()[0];
      green_scale = 255.0 / max_colour.value()[1];
      blue_scale = 255.0 / max_colour.value()[2];

      for (int i = 0; i<3; i++)
      {
        if (max_colour.value()[i] <= 0.0)
        {
          double max_col = 0.0;
          for (auto &tree : forest.trees)
          {
            for (size_t s = 1; s<tree.segments().size(); s++)
            {
              auto &segment = tree.segments()[s];
              max_col = std::max(max_col, segment.attributes[red_id + i]);
            }
          }  
          double &channel = i==0 ? red_scale : (i==1 ? green_scale : blue_scale);
          channel = 255.0 / max_col;
        }
      }
    }
    else  // then we ought to auto-scale
    {
      double max_col = 0.0;
      for (auto &tree : forest.trees)
      {
        for (size_t s = 1; s<tree.segments().size(); s++)
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
  if (smooth_option.isSet())
  {
    generateSmoothMesh(mesh, forest);
  }
  else
  {
    for (auto &tree : forest.trees)
    {
      for (size_t i = 1; i < tree.segments().size(); i++)
      {
        auto &segment = tree.segments()[i];
        ray::RGBA rgba;
        rgba.red = 127;
        rgba.green = 127;
        rgba.blue = 127;
        rgba.alpha = 255;
        if (red_id != -1)
        {
          rgba.red = uint8_t(std::min(red_scale * segment.attributes[red_id], 255.0));
          rgba.green = uint8_t(std::min(green_scale * segment.attributes[red_id + 1], 255.0));
          rgba.blue = uint8_t(std::min(blue_scale * segment.attributes[red_id + 2], 255.0));
        }
        addCylinder(mesh, segment.tip, tree.segments()[segment.parent_id].tip, segment.radius, rgba);
      }
    }
  }
  ray::writePlyMesh(forest_file.nameStub() + "_mesh.ply", mesh, true);
  if (view.isSet())
  {
    return system(("meshlab " + forest_file.nameStub() + "_mesh.ply").c_str());
  }
  return 0;
}
