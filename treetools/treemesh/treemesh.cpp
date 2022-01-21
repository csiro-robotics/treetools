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

void addCylinder(ray::Mesh &mesh, const Eigen::Vector3d &pos1, const Eigen::Vector3d &pos2, double radius)
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

  ray::Mesh mesh;
  for (auto &tree: forest.trees)
  {
    for (size_t i = 1; i<tree.segments().size(); i++)
    {
      auto &segment = tree.segments()[i];
      addCylinder(mesh, segment.tip, tree.segments()[segment.parent_id].tip, segment.radius);
    }
  }
  ray::writePlyMesh(forest_file.nameStub() + "_mesh.ply", mesh, true);  
}


