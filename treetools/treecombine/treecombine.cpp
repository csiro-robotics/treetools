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
  std::cout << "Combine multiple trees together" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treecombine trees1.txt trees2.txt trees3.txt - concatenate together if they have the same attributes" << std::endl;
  std::cout << "                                             - or concatenate attributes if they have the same data" << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method combines multiple tree files into a single file. There are currently two ways it can combine:
/// 1. the files have the same attributes - so concatenate the files
/// 2. the files have the same mandatory data - so concatenate the attributes
int main(int argc, char *argv[])
{
  ray::FileArgumentList tree_files(2);
  const bool parsed = ray::parseCommandLine(argc, argv, { &tree_files });
  if (!parsed)
  {
    usage();
  }

  ray::ForestStructure combined_forest;
  std::vector<std::string> attributes, tree_attributes;
  bool same_attributes = true, same_data = false;

  int num_combined = 0;
  for (size_t i = 0; i < tree_files.files().size(); i++)
  {
    ray::ForestStructure forest;
    if (!forest.load(tree_files.files()[i].name()))
    {
      std::cout << "file " << tree_files.files()[i].name() << " doesn't load, so skipping it" << std::endl;
      continue;
    }
    if (num_combined == 0)
    {
      attributes = forest.trees[0].attributeNames();
      tree_attributes = forest.trees[0].treeAttributeNames();
    }
    else if (num_combined == 1)
    {
      same_attributes = forest.trees[0].attributeNames() == attributes && forest.trees[0].treeAttributeNames() == tree_attributes;

      same_data = forest.trees.size() == combined_forest.trees.size();
      if (same_data)  // then check more closely...
      {
        for (size_t j = 0; j < forest.trees.size(); j++)
        {
          if (forest.trees[j].segments().size() != combined_forest.trees[j].segments().size())
          {
            same_data = false;
            break;
          }
          for (size_t k = 0; k < forest.trees[j].segments().size(); k++)
          {
            if (forest.trees[j].segments()[k].parent_id != combined_forest.trees[j].segments()[k].parent_id)
            {
              same_data = false;
              break;
            }
          }
          if (!same_data)
            break;
        }
      }
      if (!same_attributes && !same_data)
      {
        std::cerr
          << "Error: tree files have different sets of attributes and different tree structures, so cannot combine"
          << std::endl;
        usage();
      }
      if (same_attributes && same_data)
      {
        std::cerr << "Error: tree files have same attributes and same tree structures, so no need to combine"
                  << std::endl;
        usage();
      }
    }
    num_combined++;
    if (same_attributes)
    {
      combined_forest.trees.insert(combined_forest.trees.end(), forest.trees.begin(), forest.trees.end());
    }
    else if (same_data)
    {
      // If the trees share an attribute, I don't take the mean. This could mess up on large double integers
      // instead I just keep the values in the first data set; the first data set has priority
      { // first the tree attributes:
        std::vector<int> att_locations(forest.trees[0].treeAttributeNames().size(), -1);
        const auto &atts = combined_forest.trees[0].treeAttributeNames();
        for (size_t j = 0; j < forest.trees[0].treeAttributeNames().size(); j++)
        {
          const auto &it = std::find(atts.begin(), atts.end(), forest.trees[0].treeAttributeNames()[j]);
          if (it != atts.end())
          {
            att_locations[j] = (int)(it - atts.begin());
          }
        }
        for (size_t l = 0; l < att_locations.size(); l++)
        {
          const int id = att_locations[l];
          if (id == -1)
          {
            for (size_t j = 0; j < forest.trees.size(); j++)
            {
              combined_forest.trees[j].treeAttributeNames().push_back(forest.trees[j].treeAttributeNames()[l]);
              combined_forest.trees[j].treeAttributes().push_back(forest.trees[j].treeAttributes()[l]);
            }
          }
        }
      }
      { // then the segment attributes:
        std::vector<int> att_locations(forest.trees[0].attributeNames().size(), -1);
        const auto &atts = combined_forest.trees[0].attributeNames();
        for (size_t j = 0; j < forest.trees[0].attributeNames().size(); j++)
        {
          const auto &it = std::find(atts.begin(), atts.end(), forest.trees[0].attributeNames()[j]);
          if (it != atts.end())
          {
            att_locations[j] = (int)(it - atts.begin());
          }
        }
        for (size_t l = 0; l < att_locations.size(); l++)
        {
          const int id = att_locations[l];
          if (id == -1)
          {
            for (size_t j = 0; j < forest.trees.size(); j++)
            {
              combined_forest.trees[j].attributeNames().push_back(forest.trees[j].attributeNames()[l]);
              for (size_t k = 0; k < forest.trees[j].segments().size(); k++)
              {
                combined_forest.trees[j].segments()[k].attributes.push_back(forest.trees[j].segments()[k].attributes[l]);
              }
            }
          }
        }        
      }
    }
  }
  if (num_combined == 0)
  {
    std::cerr << "Error: no forest files could be loaded" << std::endl;
    usage();
  }
  combined_forest.save(tree_files.files()[0].nameStub() + "_combined.txt");
  return 0;
}
