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
  std::cout << "Scale attributes of the tree file in-place" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treescale forest.txt <attribute> 0.5 - scale <attribute> by half " << std::endl;
  std::cout << "                     <attribute1> <attribute2> <attribute3> 0.5 - scale three attributes by one value " << std::endl;
  std::cout << "                     <attribute1> <attribute2> <attribute3> 0.5,1,3 - scale three attributes by three values " << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, attribute(false), attribute2(false), attribute3(false);
  ray::DoubleArgument scale;
  ray::Vector3dArgument scale3D;
  bool format1 = ray::parseCommandLine(argc, argv, {&forest_file, &attribute, &scale});
  bool format2 = ray::parseCommandLine(argc, argv, {&forest_file, &attribute, &attribute2, &attribute3, &scale});
  bool format3 = ray::parseCommandLine(argc, argv, {&forest_file, &attribute, &attribute2, &attribute3, &scale3D});
  if (!format1 && !format2 && !format3)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }  

  auto &att = forest.trees[0].attributes();
  std::vector<std::string> atnames = {attribute.name()};
  if (format2 || format3)
  {
    atnames.push_back(attribute2.name());
    atnames.push_back(attribute3.name());
  }
  int att_ids[3] = {0,0,0};
  for (size_t i = 0; i<atnames.size(); i++)
  {
    const auto &it = std::find(att.begin(), att.end(), atnames[i]);
    if (it != att.end())
    {
      att_ids[i] = (int)(it - att.begin());
    }
    else // no colour found so lets add empty values across the whole structure
    {
      std::cout << "attribute: " << atnames[i] << " not found in the tree file " << forest_file.name() << std::endl;
      usage();
    }
  }

  for (auto &tree: forest.trees)
  {
    for (auto &segment: tree.segments())
    {
      if (format1)
        segment.attributes[att_ids[0]] *= scale.value();
      else if (format2)
      {
        segment.attributes[att_ids[0]] *= scale.value();
        segment.attributes[att_ids[1]] *= scale.value();
        segment.attributes[att_ids[2]] *= scale.value();
      }
      else 
      {
        segment.attributes[att_ids[0]] *= scale3D.value()[0];
        segment.attributes[att_ids[1]] *= scale3D.value()[1];
        segment.attributes[att_ids[2]] *= scale3D.value()[2];    
      }
    }
  }
  forest.save(forest_file.name());
}


