// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#define STB_IMAGE_IMPLEMENTATION
#include <raylib/extraction/raytrees.h>
#include <raylib/raycloud.h>
#include <raylib/raycloudwriter.h>
#include <raylib/rayforeststructure.h>
#include <raylib/rayparse.h>
#include <raylib/rayrenderer.h>
#include <raylib/raytreegen.h>
#include <cstdlib>
#include <iostream>
#include "treelib/imageread.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Paint a tree file's colour onto a segmented ray cloud." << std::endl;
  std::cout << "The cloud should be segmented by branch or by tree" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treepaint forest.txt trees_segmented.ply - paint tree colours onto segmented cloud" << std::endl;
  std::cout << "                     --max_colour 1 - specify the maximum brightness, otherwise it autoscales"
            << std::endl;
  // clang-format on
  exit(exit_code);
}

/// This method applies the tree file's colour onto the specified segmented ray cloud. It is assumed 
/// that the ray cloud was generated from rayextract and therefore its segment colouring matches the
/// section_id values within the tree file. This is how the tree file's colours are applied to the correct
/// sections of the ray cloud.
int main(int argc, char *argv[])
{
  ray::FileArgument forest_file, cloud_file;

  ray::DoubleArgument max_brightness;
  ray::OptionalKeyValueArgument max_brightness_option("max_colour", 'm', &max_brightness);

  ray::Vector3dArgument coord;
  const bool tree_format = ray::parseCommandLine(argc, argv, { &forest_file, &cloud_file }, { &max_brightness_option });
  const bool branch_format = ray::parseCommandLine(argc, argv, { &forest_file, &cloud_file }, { &max_brightness_option });
  if (!tree_format && !branch_format)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }

  const std::string attributes[4] = { "red", "green", "blue", "section_id" };
  int att_ids[4];
  auto &att = forest.trees[0].attributes();
  for (int i = 0; i < 4; i++)
  {
    const auto &it = std::find(att.begin(), att.end(), attributes[i]);
    if (it != att.end())
    {
      att_ids[i] = (int)(it - att.begin());
    }
    else
    {
      std::cerr << "Error: this function requires a " << attributes[i]
                << " field in the tree file, to match against the segmented cloud colours" << std::endl;
      usage();
    }
  }
  int segment_id = att_ids[3];
  double max_shade = 0.0;
  if (max_brightness_option.isSet())
  {
    max_shade = max_brightness.value();
  }
  else
  {
    for (auto &tree : forest.trees)
    {
      for (auto &segment : tree.segments())
      {
        for (int i = 0; i < 3; i++) 
        {
          max_shade = std::max(max_shade, segment.attributes[att_ids[i]]);
        }
      }
    }
  }
  std::string out_file = cloud_file.nameStub() + "_painted.ply";

  // finally, we need a mappinig from segment id to the segment structures...
  int num_segments = 0;
  for (auto &tree : forest.trees)
  {
    for (auto &segment : tree.segments()) num_segments = std::max(num_segments, (int)segment.attributes[segment_id]);
  }
  num_segments++;
  std::vector<ray::TreeStructure::Segment *> segments(num_segments, 0);
  for (auto &tree : forest.trees)
  {
    for (auto &segment : tree.segments())
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

  ray::CloudWriter writer;
  if (!writer.begin(out_file))
  {
    usage();
  }

  auto colour_rays = [&](std::vector<Eigen::Vector3d> &starts, std::vector<Eigen::Vector3d> &ends,
                         std::vector<double> &times, std::vector<ray::RGBA> &colours) {
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
          colour.red = (uint8_t)std::min(255.0 * seg->attributes[att_ids[0]] / max_shade, 255.0);
          colour.green = (uint8_t)std::min(255.0 * seg->attributes[att_ids[1]] / max_shade, 255.0);
          colour.blue = (uint8_t)std::min(255.0 * seg->attributes[att_ids[2]] / max_shade, 255.0);
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
  return 0;
}
