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
  std::cout << "Combine multiple trees together" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treecombine trees1.txt trees2.txt trees3.txt - concatenate together, as long as they have the same attributes" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgumentList tree_files(2);
  bool parsed = ray::parseCommandLine(argc, argv, {&tree_files});
  if (!parsed)
  {
    usage();
  }

  ray::ForestStructure combined_forest;
  std::vector<std::string> attributes;
  for (size_t i = 0; i<tree_files.files().size(); i++)
  {
    ray::ForestStructure forest;
    if (!forest.load(tree_files.files()[i].name()))
    {
      usage();
    }  
    if (i == 0)
    {
      attributes = forest.trees[0].attributes();
    }
    else
    {
      for (size_t j = 0; j<forest.trees[0].attributes().size(); j++)
      {
        if (forest.trees[0].attributes()[j] != attributes[j])
        {
          std::cerr << "Error: tree files have different sets of attributes, cannot comcatenate" << std::endl;
          usage();
        }
      }
    }
    combined_forest.trees.insert(combined_forest.trees.end(), forest.trees.begin(), forest.trees.end());
  }

  combined_forest.save(tree_files.files()[0].nameStub() + "_combined.txt");
}


