// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treelib/treeutils.h"
#include <raylib/raycloud.h>
#include <raylib/rayforeststructure.h>
#include <raylib/rayparse.h>
#include <raylib/rayrenderer.h>
#include <raylib/raytreegen.h>
#include <cstdlib>
#include <iostream>
#include "raylib/raytreegen.h"
#include "raylib/rayprogress.h"
#include "raylib/rayprogressthread.h"
//#define STB_IMAGE_IMPLEMENTATION
#include "treelib/imagewrite.h"
#include "raylib/raylibconfig.h"

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "render a tree file to an image" << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treerender trees.txt                 - render by segment colour" << std::endl;
  std::cout << "                  --max_colour 1     - colour using this as the maximum component value" << std::endl;
  std::cout << "treerender trees.txt height          - render by height (greyscale over range)" << std::endl;
  std::cout << "                     volume          - render by volume (greyscale over range)" << std::endl;
//  std::cout << "                     surface_area    - render by surface area" << std::endl;
  std::cout << "                     --rgb           - render greyscale as a red->green->blue colour gradient around its range" << std::endl;
  std::cout << "                  --resolution 512   - default resolution of longest axis" << std::endl;
  std::cout << "                  --pixel_width 0.1  - pixel width in metres as alternative to resolution setting" << std::endl;
  std::cout << "                  --grid_width 100   - fit to a square grid of this width, with one grid cell centre at 0,0" << std::endl;
  std::cout << "                  --crop x,y,rx,ry   - crop to window centred at x,y with radius (half-width) rx,ry" << std::endl;
  std::cout << "                  --output image.hdr - set output file (supported image types: .jpg, .png, .bmp, .tga, .hdr)" << std::endl;
  std::cout << "                  --num_subvoxels 8  - used for volume estimation" << std::endl;
  std::cout << "                  --georeference name.proj- projection file name, to output (geo)tif file. " << std::endl;
  // clang-format on
  exit(exit_code);
}

Eigen::Vector3d gradient(double shade)
{
  Eigen::Vector3d col = ray::redGreenBlueGradient(shade);
  if (shade < 0.05) // fade to black
    return 20.0*shade * col;
  return col;
}

struct Capsule
{
  Eigen::Vector3d v1, v2;
  double min_height {-1e10};
  double radius;

  bool overlaps(const Eigen::Vector3d &pos)
  {
    Eigen::Vector3d vec = v2 - v1;
    Eigen::Vector3d closest = v1 + vec*(pos - v1).dot(vec)/vec.squaredNorm();
    return (closest - pos).squaredNorm() <= radius*radius;
  }
  double rayIntersectionDepth(const Eigen::Vector3d &start, const Eigen::Vector3d &end)
  {
    Eigen::Vector3d ray = end - start;
    Eigen::Vector3d dir = v2 - v1;
    double length = dir.norm();
    if (length > 0.0)
    {
      dir /= length;      
    }

    // cylinder part:
    double cylinder_intersection1 = 1e10;
    Eigen::Vector3d up = dir.cross(ray);
    double mag = up.norm();
    if (mag > 0.0) // two rays are not inline
    {
      up /= mag;
      double gap = std::abs((start - v1).dot(up));
      if (gap > radius)
        return 0.0;

      Eigen::Vector3d lateral_dir = ray - dir * ray.dot(dir);
      double lateral_length = lateral_dir.norm();
      double d_mid = (v1 - start).dot(lateral_dir) / ray.dot(lateral_dir);
      double shift = std::sqrt(radius*radius - gap*gap) / lateral_length;
      double d_min = d_mid - shift;
      double d1 = (start + ray*d_min - v1).dot(dir) / length;
      if (d1 > 0.0 && d1 < 1.0)
      {
        cylinder_intersection1 = d_min;
      }
    }

    // the spheres part:
    double ray_length = ray.norm();
    double sphere_intersection1[2] = {1e10, 1e10};
    Eigen::Vector3d ends[2] = {v1, v2};
    for (int e = 0; e<2; e++)
    {
      double mid_d = (ends[e] - start).dot(ray) / (ray_length * ray_length);
      Eigen::Vector3d shortest_dir = (ends[e] - start) - ray * mid_d;
      double shortest_sqr = shortest_dir.squaredNorm();
      if (shortest_sqr < radius*radius)
      {
        double shift = std::sqrt(radius * radius - shortest_sqr) / ray_length;
        sphere_intersection1[e] = mid_d - shift;
      }
    }

    // combining together
    double closest_d = std::min( {cylinder_intersection1, sphere_intersection1[0], sphere_intersection1[1]} );
    if (closest_d == 1e10)
      return 0.0;
    return closest_d * ray_length;
  }
};

/// This method combines multiple tree files into a single file. There are currently two ways it can combine:
/// 1. the files have the same attributes - so concatenate the files
/// 2. the files have the same mandatory data - so concatenate the attributes
int main(int argc, char *argv[])
{
  ray::FileArgument tree_file, output_file, projection_file;
  ray::KeyChoice style({ "height", "volume", "surface_area", "plant_density" }); 
  ray::OptionalFlagArgument rgb_flag("rgb", 'r');
  ray::DoubleArgument pixel_width_arg(0.001, 100000.0), grid_width(0.001, 100000.0), max_brightness(0.000001, 100000000.0);
  ray::IntArgument num_subvoxels(1,1000, 8), resolution(1, 20000, 512);
  ray::Vector4dArgument crop_posrad;
  ray::OptionalKeyValueArgument output_image_option("output", 'o', &output_file);
  ray::OptionalKeyValueArgument pixel_width_option("pixel_width", 'p', &pixel_width_arg);
  ray::OptionalKeyValueArgument resolution_option("resolution", 'r', &resolution);
  ray::OptionalKeyValueArgument grid_width_option("grid_width", 'g', &grid_width);
  ray::OptionalKeyValueArgument crop_option("crop", 'c', &crop_posrad);
  ray::OptionalKeyValueArgument max_brightness_option("max_colour", 'm', &max_brightness);
  ray::OptionalKeyValueArgument num_subvoxels_option("num_subvoxels", 'n', &num_subvoxels);
  ray::OptionalKeyValueArgument projection_file_option("georeference", 'g', &projection_file);

  const bool standard_format = ray::parseCommandLine(argc, argv, { &tree_file }, {&output_image_option, &grid_width_option, &resolution_option, &pixel_width_option, &crop_option, &max_brightness_option, &projection_file_option});
  const bool variant_format = ray::parseCommandLine(argc, argv, { &tree_file, &style }, {&output_image_option, &grid_width_option, &resolution_option, &pixel_width_option, &crop_option, &num_subvoxels_option, &rgb_flag, &projection_file_option});
  if (!standard_format && !variant_format)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(tree_file.name()))
  {
    usage();
  }

  // if colouring the mesh:
  int red_id = -1;
  double red_scale = 1.0;
  double green_scale = 1.0;
  double blue_scale = 1.0;

  // first, get a pixel width:
  const double big = 1e10;
  Eigen::Vector3d min_bound(big,big,big), max_bound(-big,-big,-big);
  for (auto &tree: forest.trees)
  {
    for (auto &segment: tree.segments())
    {
      min_bound = ray::minVector(min_bound, segment.tip);
      max_bound = ray::maxVector(max_bound, segment.tip);
    }
  }
  Eigen::Vector3d extent = max_bound - min_bound;
  double pixel_width = pixel_width_arg.value();
  if (!pixel_width_option.isSet())
  {
    double length = std::max(extent[0], extent[1]);
    pixel_width = length / resolution.value();
  }
  int width = (int)std::round(extent[0] / pixel_width);
  int height = (int)std::round(extent[1] / pixel_width);
  if (crop_option.isSet()) // adjust min_bound to be well-aligned
  {
    Eigen::Vector4d pr = crop_posrad.value();
    min_bound = Eigen::Vector3d(pr[0]-pr[2], pr[1]-pr[3], min_bound[2]);
    max_bound = Eigen::Vector3d(pr[0]+pr[2], pr[1]+pr[3], max_bound[2]);
    if (!pixel_width_option.isSet())
      pixel_width = 2.0 * std::max(pr[2], pr[3]) / (double)resolution.value();
    width = (int)std::round(2.0*pr[2]/pixel_width);
    height = (int)std::round(2.0*pr[3]/pixel_width);
  }
  else if (grid_width_option.isSet()) // adjust min_bound to be well-aligned
  {
    Eigen::Vector3d mid = (min_bound + max_bound)/2.0;
    min_bound[0] = grid_width.value() * std::round(mid[0] / grid_width.value()) - 0.5*grid_width.value();
    min_bound[1] = grid_width.value() * std::round(mid[1] / grid_width.value()) - 0.5*grid_width.value();
    if (!pixel_width_option.isSet())
      pixel_width = grid_width.value() / (double)resolution.value();
    width = height = (int)std::round(grid_width.value()/pixel_width);
  }

  std::string image_file = output_image_option.isSet() ? output_file.name() : tree_file.nameStub() + ".png";
  const std::string image_ext = ray::getFileNameExtension(image_file);
  const bool is_hdr = image_ext == "hdr" || image_ext == "tif";

  std::vector<ray::RGBA> pixel_colours;
  std::vector<float> float_pixel_colours;
  if (is_hdr)
    float_pixel_colours.resize(3 * width * height, 0.0);
  else
    pixel_colours.resize(width * height, ray::RGBA(0,0,0,0));

  if (standard_format || (variant_format && style.selectedKey() == "height"))
  {
    if (standard_format)
    {
      auto &att = forest.trees[0].attributeNames();
      const auto &it = std::find(att.begin(), att.end(), "red");
      if (it == att.end())
      {
        std::cerr << "Error: cannot find colour in trees file" << std::endl;
        usage();
      }
      red_id = static_cast<int>(it - att.begin());
      if (red_id != -1)
      {
        // option to rescale overall brightness
        if (max_brightness_option.isSet())
        {
          red_scale = green_scale = blue_scale = 255.0 / max_brightness.value();
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
    }
    // we need a depth buffer
    std::vector<double> depths(width*height, big);
    for (auto &tree: forest.trees)
    {
      for (size_t i = 1; i<tree.segments().size(); i++)
      {
        auto &segment = tree.segments()[i];
        Capsule capsule;
        capsule.v2 = segment.tip;
        capsule.v1 = tree.segments()[segment.parent_id].tip;
        capsule.radius = segment.radius + pixel_width/2.0;
        if (segment.parent_id == 0) // need to clip capsule's lower cap at ground level
        {
          capsule.min_height = capsule.v1[2];
        }
        Eigen::Vector3d min_caps = ray::minVector(capsule.v1, capsule.v2) - Eigen::Vector3d(capsule.radius, capsule.radius, 0);
        Eigen::Vector3d max_caps = ray::maxVector(capsule.v1, capsule.v2) + Eigen::Vector3d(capsule.radius, capsule.radius, 0);
        Eigen::Vector3i mins = ((min_caps - min_bound) / pixel_width).cast<int>();
        Eigen::Vector3i maxs = ((max_caps - min_bound) / pixel_width).cast<int>() + Eigen::Vector3i(1,1,1);
        mins = ray::maxVector(Eigen::Vector3i(0,0,0), mins);
        maxs = ray::minVector(maxs, Eigen::Vector3i(width, height, 0));
        for (int x = mins[0]; x < maxs[0]; x++)
        {
          for (int y = mins[1]; y < maxs[1]; y++)
          {
            Eigen::Vector3d top = Eigen::Vector3d((double)x + 0.5, (double)y + 0.5, 0.0)*pixel_width + min_bound;
            top[2] = max_bound[2] + pixel_width;
            Eigen::Vector3d bottom = top;
            bottom[2] = min_bound[2] - pixel_width;

            double depth = capsule.rayIntersectionDepth(top, bottom);
            if (depth > 0.0)
            {
              int ind = x + width*y;
              depths[ind] = std::min(depths[ind], depth);
              if (depths[ind] == depth) // deepest
              {
                if (variant_format)
                {
                  double shade = std::max(0.0, std::min(1.0 - depth / (max_bound[2] - min_bound[2]), 1.0));
                  if (rgb_flag.isSet())
                  {
                    Eigen::Vector3d col = gradient(shade);
                    if (is_hdr)
                    {
                      float_pixel_colours[3*ind + 0] = (float)col[0];
                      float_pixel_colours[3*ind + 1] = (float)col[1];
                      float_pixel_colours[3*ind + 2] = (float)col[2];
                    }
                    else
                      pixel_colours[ind] = ray::RGBA((uint8_t)(col[0] * 255.0),(uint8_t)(col[1] * 255.0),(uint8_t)(col[2] * 255.0),255); 
                  }
                  else
                  {
                    uint8_t shade255 = (uint8_t)(shade * 255.0);
                    if (is_hdr)
                      float_pixel_colours[3*ind + 0] = float_pixel_colours[3*ind + 1] = float_pixel_colours[3*ind + 2] = (float)shade;
                    else
                      pixel_colours[ind] = ray::RGBA(shade255,shade255,shade255,255); 
                  }
                }
                else // segment colour
                {
                  Eigen::Vector3d col(segment.attributes[red_id], segment.attributes[red_id + 1], segment.attributes[red_id + 2]);
                  if (is_hdr)
                  {
                    float_pixel_colours[3*ind + 0] = (float)col[0];
                    float_pixel_colours[3*ind + 1] = (float)col[1];
                    float_pixel_colours[3*ind + 2] = (float)col[2];
                  }
                  else
                  {
                    pixel_colours[ind] = ray::RGBA((uint8_t)(col[0]*red_scale), (uint8_t)(col[1]*green_scale), (uint8_t)(col[2]*blue_scale), 255);                 
                  }
                }
              }
            }
          }
        }
      }
    }    
  }
  else if (style.selectedKey() == "volume")
  {
    // first, calculate capsules that intersect each pixel
    std::vector<std::vector<Capsule>> capsule_grid(width*height);
    for (auto &tree: forest.trees)
    {
      for (size_t i = 1; i<tree.segments().size(); i++)
      {
        auto &segment = tree.segments()[i];
        Capsule capsule;
        capsule.v2 = segment.tip;
        capsule.v1 = tree.segments()[segment.parent_id].tip;
        capsule.radius = segment.radius;
        Eigen::Vector3d min_caps = ray::minVector(capsule.v1, capsule.v2) - Eigen::Vector3d(capsule.radius, capsule.radius, 0);
        Eigen::Vector3d max_caps = ray::maxVector(capsule.v1, capsule.v2) + Eigen::Vector3d(capsule.radius, capsule.radius, 0);
        Eigen::Vector3i mins = ((min_caps - min_bound) / pixel_width).cast<int>();
        Eigen::Vector3i maxs = ((max_caps - min_bound) / pixel_width).cast<int>() + Eigen::Vector3i(1,1,1);
        mins = ray::maxVector(Eigen::Vector3i(0,0,0), mins);
        maxs = ray::minVector(maxs, Eigen::Vector3i(width, height, 0));
        for (int x = mins[0]; x < maxs[0]; x++)
        {
          for (int y = mins[1]; y < maxs[1]; y++)
          {
            capsule_grid[x + width*y].push_back(capsule);
          }
        }
      }
    }

    int n = num_subvoxels.value();
    // now use a grid memory structure to avoid overlap issues
    double subpixel_width = pixel_width / (double)n;
    double subpixel_volume = subpixel_width*subpixel_width*subpixel_width;
    int num_vertical = (int)std::ceil((max_bound[2] - min_bound[2])/subpixel_width); // this will be large!!
    std::vector<int> counts[2];
    int max_count[2] = {0,0};
    
    double ds[2] = {0.25, 0.75};
    for (int phase = 0; phase<2; phase++)
    {
      double delta = ds[phase];
      counts[phase].resize(width*height, 0);
      ray::Progress progress;
      ray::ProgressThread progress_thread(progress);    
      progress.begin("calculate volumes " + std::to_string(phase+1) + "/2: ", width/10);
      for (int x = 0; x<width; x++)
      {
        for (int y = 0; y<height; y++)
        {
          Eigen::Vector3d pixel_min_bound = min_bound + pixel_width*Eigen::Vector3d(x,y,0);
          std::vector<bool> subpixels(n*n*num_vertical, false);
          int count = 0;
          int ind = x + width*y;
          for (auto &capsule: capsule_grid[ind])
          {
            Eigen::Vector3d min_caps = ray::minVector(capsule.v1, capsule.v2) - Eigen::Vector3d(capsule.radius, capsule.radius, capsule.radius);
            Eigen::Vector3d max_caps = ray::maxVector(capsule.v1, capsule.v2) + Eigen::Vector3d(capsule.radius, capsule.radius, capsule.radius);
            Eigen::Vector3i mins = ((min_caps - pixel_min_bound) / subpixel_width).cast<int>();
            Eigen::Vector3i maxs = ((max_caps - pixel_min_bound) / subpixel_width).cast<int>() + Eigen::Vector3i(1,1,1);
            mins = ray::maxVector(Eigen::Vector3i(0,0,0), mins);
            maxs = ray::minVector(maxs, Eigen::Vector3i(n, n, num_vertical));
            for (int xx = mins[0]; xx < maxs[0]; xx++)
            {
              for (int yy = mins[1]; yy < maxs[1]; yy++)
              {
                for (int zz = mins[2]; zz < maxs[2]; zz++)
                {
                  if (subpixels[xx + n*yy + n*n*zz]) // already set
                    continue;
                  Eigen::Vector3d pos = Eigen::Vector3d((double)xx+delta,(double)yy+delta,(double)zz+delta)*subpixel_width + pixel_min_bound;
                  if (pos[2] < capsule.min_height) // below ground
                    continue;
                  if (capsule.overlaps(pos))
                  {
                    subpixels[xx + n*yy + n*n*zz] = true;       
                    count++;
                  }           
                }
              }
            }
          }
          counts[phase][ind] = count;
          max_count[phase] = std::max(max_count[phase], count);
        }
        if (!(x%10))
          progress.increment();
      }
      progress.end();
      progress_thread.requestQuit();
      progress_thread.join();
    }
    double total_error = 0.0;
    double total_volume = 0.0;
    for (int x = 0; x<width; x++)
    {
      for (int y = 0; y<height; y++)
      {
        int ind = x + width*y;
        double error = subpixel_volume * (double)(counts[0][ind] - counts[1][ind])/2.0;
        total_error += std::abs(error);
        double volume = subpixel_volume * (double)(counts[0][ind] + counts[1][ind])/2.0;
        total_volume += volume;
        double max_volume = subpixel_volume * (double)(max_count[0] + max_count[1]);
        double shade = std::max(0.0, std::min(volume/max_volume, 1.0));
        if (rgb_flag.isSet())
        {
          Eigen::Vector3d col = gradient(shade);
          if (is_hdr)
          {
            float_pixel_colours[3*ind + 0] = (float)col[0];
            float_pixel_colours[3*ind + 1] = (float)col[1];
            float_pixel_colours[3*ind + 2] = (float)col[2];
          }
          else
            pixel_colours[ind] = ray::RGBA((uint8_t)(col[0] * 255.0),(uint8_t)(col[1] * 255.0),(uint8_t)(col[2] * 255.0),255); 
        }
        else
        {
          uint8_t shade255 = (uint8_t)(shade * 255.0);
          if (is_hdr)
            float_pixel_colours[3*ind + 0] = float_pixel_colours[3*ind + 1] = float_pixel_colours[3*ind + 2] = (float)volume;
          else
            pixel_colours[ind] = ray::RGBA(shade255,shade255,shade255,255);         
        }
      }
    }
    std::cout << "subpixel width: " << subpixel_width << " m, total volume: " << total_volume << " m^3, pixel volume % error: " << 100.0*total_error/total_volume << "%" << std::endl;
  }
  else
  {
    std::cerr << "Error: style " << style.selectedKey() << " not yet supported" << std::endl;
    usage();
  }

  std::cout << "outputting image: " << image_file << std::endl;

  // write the image depending on the file format
  const char *image_name = image_file.c_str();
  stbi_flip_vertically_on_write(1);
  if (image_ext == "png")
    stbi_write_png(image_name, width, height, 4, (void *)&pixel_colours[0], 4 * width);
  else if (image_ext == "bmp")
    stbi_write_bmp(image_name, width, height, 4, (void *)&pixel_colours[0]);
  else if (image_ext == "tga")
    stbi_write_tga(image_name, width, height, 4, (void *)&pixel_colours[0]);
  else if (image_ext == "jpg")
    stbi_write_jpg(image_name, width, height, 4, (void *)&pixel_colours[0], 100);  // 100 is maximal quality
  else if (image_ext == "hdr")
    stbi_write_hdr(image_name, width, height, 3, &float_pixel_colours[0]);
#if RAYLIB_WITH_TIFF 
  else if (image_ext == "tif")
  {
    // obtain the origin offsets
    const Eigen::Vector3d origin(0, 0, 0);
    const Eigen::Vector3d pos = -(origin - min_bound);
    const double x = pos[0], y = pos[1] + static_cast<double>(height) * pixel_width;
    // generate the geotiff file
    ray::writeGeoTiffFloat(image_file, width, height, &float_pixel_colours[0], pixel_width, false, projection_file.name(), x, y);
  }
#endif
  else 
  {
    std::cerr << "Error: output file extension " << image_ext << " not supported" << std::endl;
    usage();
  }

  return 0;
}
