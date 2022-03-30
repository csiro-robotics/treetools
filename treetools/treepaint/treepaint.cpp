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
#include <raylib/extraction/raytrees.h>
#include <raylib/rayparse.h>
#include <raylib/raycloud.h>
#include <raylib/rayrenderer.h>
#include <raylib/raytreegen.h>
#include <raylib/raycloudwriter.h>
#include <cstdlib>
#include <iostream>

void usage(int exit_code = 1)
{
  std::cout << "Paint a tree file's colour onto a segmented point cloud." << std::endl;
  std::cout << "The cloud should be segmented by branch or by tree" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treepaint trees forest.txt trees_segmented.ply - paint tree colours onto segmented cloud" << std::endl;
  std::cout << "treepaint branches forest.txt branches_segmented.ply - paint branch colours onto branch segmented cloud" << std::endl;
  exit(exit_code);
}

// Read in a ray cloud and convert it into an array for topological optimisation
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, cloud_file;
  ray::TextArgument trees_text("trees"), branches_text("branches");
  ray::Vector3dArgument coord;
  bool tree_format = ray::parseCommandLine(argc, argv, {&trees_text, &forest_file, &cloud_file});
  bool branch_format = ray::parseCommandLine(argc, argv, {&branches_text, &forest_file, &cloud_file});
  if (!tree_format && !branch_format)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }  

  std::string attributes[4] = {"red", "green", "blue", "section_id"};
  int att_ids[4];
  auto &att = forest.trees[0].attributes();
  for (int i = 0; i<4; i++)
  {
    const auto &it = std::find(att.begin(), att.end(), attributes[i]);
    if (it != att.end())
    {
      att_ids[i] = (int)(it - att.begin());
    }
    else
    {
      std::cerr << "Error: this function requires a " << attributes[i] << " field in the tree file, to match against the segmented cloud colours" << std::endl;
      usage();
    }
  }
  int segment_id = att_ids[3];
  std::string out_file = cloud_file.nameStub() + "_painted.ply";

  // finally, we need a mappinig from segment id to the segment structures...
  int num_segments = 0;
  for (auto &tree: forest.trees)
  {
    for (auto &segment: tree.segments())
      num_segments = std::max(num_segments, (int)segment.attributes[segment_id]);
  }
  num_segments++;
  std::vector<ray::TreeStructure::Segment *> segments(num_segments, 0);
  for (auto &tree: forest.trees)
  {
    for (auto &segment: tree.segments())
    {
      int id = (int)segment.attributes[segment_id];
      if (id < 0 || id >= num_segments)
      {
        std::cerr << "bad segment id: " << id << std::endl;
        usage();
      }
      segments[id] = &segment;
    }
  }

  if (branch_format)
  {
    std::cerr << "branches not supported yet" << std::endl;
    usage();
  }

/*  ray::Cloud cloud;
  if (!cloud.load(cloud_file.name()))
    usage();
  
  for (auto &colour: cloud.colours)
  {
    if (colour.alpha == 0)
      continue;        
    int seg_id = ray::convertColourToInt(colour);
    if (seg_id != -1)
    {
      ray::TreeStructure::Segment *seg = segments[seg_id];
      if (seg)
      {
        colour.red = (uint8_t)std::min(seg->attributes[att_ids[0]], 255.0);
        colour.green = (uint8_t)std::min(seg->attributes[att_ids[1]], 255.0);
        colour.blue = (uint8_t)std::min(seg->attributes[att_ids[2]], 255.0);
      }
    }
  }
  cloud.save(out_file);
  return 1;*/

  ray::CloudWriter writer;
  if (!writer.begin(out_file))
    usage();

  auto colour_rays = [&]
    (std::vector<Eigen::Vector3d> &starts, std::vector<Eigen::Vector3d> &ends, 
      std::vector<double> &times, std::vector<ray::RGBA> &colours)
  {
    if (tree_format)
    {
      for (auto &colour : colours)
      {
        int seg_id = ray::convertColourToInt(colour);
        if (seg_id == -1)
        {
          colour.red = colour.green = colour.blue = 0;
        }
        else 
        {
          ray::TreeStructure::Segment *seg = segments[seg_id];
          if (seg)
          {
            colour.red = (uint8_t)std::min(seg->attributes[att_ids[0]], 255.0);
            colour.green = (uint8_t)std::min(seg->attributes[att_ids[1]], 255.0);
            colour.blue = (uint8_t)std::min(seg->attributes[att_ids[2]], 255.0);
          }
        }
      }
    }
    writer.writeChunk(starts, ends, times, colours);
  };
  
  if (!ray::Cloud::read(cloud_file.name(), colour_rays))
  {
    usage();
  }
  writer.end();
}


