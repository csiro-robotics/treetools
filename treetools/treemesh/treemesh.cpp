// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#include <raylib/rayforestgen.h>
#include <raylib/raymesh.h>
#include <raylib/rayparse.h>
#include <raylib/rayply.h>
#include <cstdlib>
#include <iostream>

void usage(int exit_code = 1)
{
  std::cout << "Export the trees to a mesh" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treemesh forest.txt" << std::endl;
  exit(exit_code);
}

void addCylinder(ray::Mesh &mesh, const Eigen::Vector3d &pos1, const Eigen::Vector3d &pos2, double radius, ray::RGBA rgba)
{
  int n = (int)mesh.vertices().size();
  Eigen::Vector3i N(n,n,n);
  std::vector<Eigen::Vector3i> &indices = mesh.indexList();
  std::vector<Eigen::Vector3d> vertices(14);

  Eigen::Vector3d dir = (pos2 - pos1).normalized();
  Eigen::Vector3d diag(1,2,3);
  Eigen::Vector3d side1 = dir.cross(diag).normalized();
  Eigen::Vector3d side2 = side1.cross(dir);
  vertices.resize(14);
  vertices[12] = pos1 - radius*dir;
  vertices[13] = pos2 + radius*dir;
  for (int i = 0; i<6; i++)
  {
    const double pi = 3.14156;
    double angle = (double)i * 2.0*pi/6.0;
    double angle2 = angle + pi/6.0;
    vertices[i] = pos1 + radius * (side1*std::sin(angle) + side2*std::cos(angle));
    vertices[i+6] = pos2 + radius * (side1*std::sin(angle2) + side2*std::cos(angle2));
    indices.push_back(N+Eigen::Vector3i(12, i, (i+1)%6)); // start end
    indices.push_back(N+Eigen::Vector3i(13, (i+1)%6 + 6, i+6));
    indices.push_back(N+Eigen::Vector3i(i, i+6, (i+1)%6));
    indices.push_back(N+Eigen::Vector3i((i+1)%6 + 6, (i+1)%6, i+6));
  }
  mesh.vertices().insert(mesh.vertices().end(), vertices.begin(), vertices.end());
  for (int i = 0; i<vertices.size(); i++)
    mesh.colours().push_back(rgba);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file;
  bool parsed = ray::parseCommandLine(argc, argv, {&forest_file});
  if (!parsed)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }  
  std::cout << "number of trees: " << forest.trees.size() << " num segments of first tree: " << forest.trees[0].segments().size() << std::endl;
  if (forest.trees[0].segments().size() == 1)
  {
    std::cerr << "Error: currently does not support exporting trunks only to a mesh" << std::endl;
    usage();
  }

    // if colouring the mesh:
  int red_id = -1;
  auto &att = forest.trees[0].attributes();
  const auto &it = std::find(att.begin(), att.end(), "red");
  if (it != att.end())
  {
    red_id = (int)(it - att.begin());
  }
  double col_scale = 1.0;
  if (red_id != -1)
  {
    bool is_int8 = true;
    auto &segment0 = forest.trees[0].segments()[0];
    if (std::fmod(segment0.attributes[red_id], 1.0) > 1e-10 || segment0.attributes[red_id]>255.0)
      is_int8 = false;
    if (!is_int8) // then we ought to scale it
    {
      double max_col = 0.0;
      for (auto &tree: forest.trees)
      {
        for (auto &segment: tree.segments())
        {
          for (int i = 0; i<3; i++)
            max_col = std::max(max_col, segment.attributes[red_id+i]);
        }
      }
      col_scale = 255.0 / max_col;
      std::cout << "tree colour values are not 0-255 integers, so rescaling according to maximum colour, by " << 1.0/max_col << std::endl;
    }
  }
  ray::Mesh mesh;
  for (auto &tree: forest.trees)
  {
    for (size_t i = 1; i<tree.segments().size(); i++)
    {
      auto &segment = tree.segments()[i];
      ray::RGBA rgba;
      rgba.red = 127; rgba.green = 127; rgba.blue = 127; rgba.alpha = 255;
      if (red_id != -1)
      {
        rgba.red = uint8_t(col_scale * segment.attributes[red_id]);
        rgba.green = uint8_t(col_scale * segment.attributes[red_id+1]);
        rgba.blue = uint8_t(col_scale * segment.attributes[red_id+2]);
      }
      addCylinder(mesh, segment.tip, tree.segments()[segment.parent_id].tip, segment.radius, rgba);
    }
  }
  ray::writePlyMesh(forest_file.nameStub() + "_mesh.ply", mesh, true);  
}


