// Copyright (c) 2020
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe

#include "raylib/raycloud.h"
#include "raylib/raymesh.h"
#include "raylib/rayply.h"
#include "raylib/rayforeststructure.h"
#include <vector>
#include <gtest/gtest.h>
#include <cstdlib>

/// Tree tools testing framework. In each test, the statistics of the resulting clouds are compared to the statistics
/// of the tree file when it was confirmed to be operating correctly. 
namespace raytest
{
  /// Issues the specified system command, including the required prefix on non-windows systems.
  int command(const std::string &system_command)
  {
    #ifdef _WIN32
    return system(system_command);
    #else
    return system(("./" + system_command).c_str());
    #endif // _WIN32
  }

  /// Issues the specified system command, allowing global functions to be called.
  int global_command(const std::string &system_command)
  {
    #ifdef _WIN32
    return system(system_command);
    #else
    return system((system_command).c_str());
    #endif // _WIN32
  }

  /// Issues the command to copy a file, which is a platform dependent system command.
  int copy(const std::string &copy_command)
  {
    #ifdef _WIN32
    return system("copy " + copy_command);
    #else
    return system(("cp " + copy_command).c_str());
    #endif // _WIN32
  }

  /// Compare the statistical (1st and 2nd order) moments of the two ray clouds. This almost surely
  /// detects differing clouds, and always equal clouds, given a tolerance @c eps.
  void compareMoments(const Eigen::ArrayXd &m1, const std::vector<double> &m2, double eps = 0.1)
  {
    for (size_t i = 0; i<m2.size(); i++)
    {
      EXPECT_GT(m1[i], m2[i]-eps);
      EXPECT_LT(m1[i], m2[i]+eps);
    }
  }

  /// Colour a tree according to the branch lengths
  TEST(Basic, TreeColour)
  {
    EXPECT_EQ(command("treecreate forest 1"), 0);
    EXPECT_EQ(command("treeinfo forest.txt"), 0);
    EXPECT_EQ(command("treecolour forest_info.txt length"), 0);
    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("forest_info_coloured.txt"));
    compareMoments(forest.getMoments(), {20, 25.8189, 908.824, 1.53544, 0.129849, 2.59429, 14, 91888, 10.4244});  
  }

  /// Create two trees, then combine them
  TEST(Basic, TreeCombine)
  {
    EXPECT_EQ(command("treecreate tree 1"), 0);
    EXPECT_EQ(copy("tree.txt tree2.txt"), 0);
    EXPECT_EQ(command("treecreate tree 2"), 0);
    EXPECT_EQ(command("treecombine tree.txt tree2.txt"), 0);
    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("tree_combined.txt"));
    compareMoments(forest.getMoments(), {2, 0, 0, 0.214118, 0.0229267, 0.574374, 0, 0, 0});
  }
  
  /// Create a tree and create a forest
  TEST(Basic, TreeCreate)
  {
    EXPECT_EQ(command("treecreate tree 3"), 0);
    ray::ForestStructure forest;
    forest.load("tree.txt");
    compareMoments(forest.getMoments(), {1, 0, 0, 0.104859, 0.0109953, 0.276382, 0, 0, 0});
    
    EXPECT_EQ(command("treecreate forest 2"), 0);
    ray::ForestStructure forest2;
    EXPECT_TRUE(forest2.load("forest.txt"));
    compareMoments(forest2.getMoments(), {20, 34.3553, 1061.61, 1.51633, 0.128301, 2.60812, 0, 0, 0});
  }
  
  /// Create a forest, then decimate
  TEST(Basic, TreeDecimate)
  {
    EXPECT_EQ(command("treecreate forest 3"), 0);
    EXPECT_EQ(command("treedecimate forest.txt 3 segments"), 0);
    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("forest_decimated.txt"));
    compareMoments(forest.getMoments(), {20, 31.5473, 974.846, 1.52272, 0.129441, 2.66069, 0, 0, 0});
  }
  
  /// Difference between two forests
  TEST(Basic, TreeDiff)
  {
    EXPECT_EQ(command("treecreate forest 1"), 0);
    EXPECT_EQ(copy("forest.txt forest2.txt"), 0);
    EXPECT_EQ(command("treerotate forest.txt 0,0,3"), 0);
    EXPECT_EQ(command("treediff forest.txt forest2.txt"), 0);
    /// TODO: comparison not implemented, so just tests that it doesn't return a bad value
  }

  /// create a raycloud forest, extract the trees, then set the foliage density of the raycloud at each branch 
  /// of the tree in forest_foliage.txt, and also save this as a shaded ray cloud in forest_densities.ply
  TEST(Basic, TreeFoliage)
  {
    EXPECT_EQ(global_command("raycreate forest 14"), 0);
    EXPECT_EQ(global_command("rayextract terrain forest.ply"), 0);
    EXPECT_EQ(global_command("rayextract trees forest.ply forest_mesh.ply"), 0);
    EXPECT_EQ(command("treefoliage forest_trees.txt forest.ply 0.3"), 0);

    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("forest_trees_foliage.txt"));
    compareMoments(forest.getMoments(), {20, 22.4323, 1037.21, 1.05798, 0.0646387, 0.917908, 3, 86012, 1960.36});
    ray::Cloud cloud;
    EXPECT_TRUE(cloud.load("forest_densities.ply"));
    compareMoments(cloud.getMoments(), {0.273292, 0.432565, 1.74031, 5.7036, 5.6054, 0.622008, 0.341838, 0.406296, 3.09499, 5.75313, 5.69983, 3.16611, 63.8625, 36.8713, 0.0936766, 0.0936766, 0.0936766, 1, 0.156845, 0.156845, 0.156845, 0});
  }

  /// Create a forest, then grow it
  TEST(Basic, TreeGrow)
  {
    EXPECT_EQ(command("treecreate forest 5"), 0);
    EXPECT_EQ(command("treegrow forest.txt 3 years"), 0);
    ray::ForestStructure forest;
    forest.load("forest_grown.txt");
    compareMoments(forest.getMoments(), {20, 46.4312, 681.105, 1.61261, 0.142173, 3.61476, 0, 0, 0});

    EXPECT_EQ(command("treegrow forest.txt -2 years"), 0);
    ray::ForestStructure forest2;
    EXPECT_TRUE(forest2.load("forest_grown.txt"));
    compareMoments(forest2.getMoments(), {20, 46.4312, 681.105, 1.41261, 0.111921, 1.80229, 0, 0, 0});
  }  

  /// Create a forest then get info on it
  TEST(Basic, TreeInfo)
  {
    EXPECT_EQ(command("treecreate forest 6"), 0);
    EXPECT_EQ(command("treeinfo forest.txt"), 0);
    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("forest_info.txt"));
    compareMoments(forest.getMoments(), {20.000, 22.294, 944.819, 1.534, 0.136, 2.909, 11.000, 43233.000, 9.601});
  }  

  /// Create a forest then mesh it
  TEST(Basic, TreeMesh)
  {
    EXPECT_EQ(command("treecreate forest 13"), 0);
    EXPECT_EQ(command("treemesh forest.txt"), 0);
    ray::Mesh mesh;
    EXPECT_TRUE(ray::readPlyMesh("forest_mesh.ply", mesh));
    compareMoments(mesh.getMoments(), {0.980902, -0.212539, 5.18355, 6.22688, 6.21057, 2.09305});
  }  

  /// Create a raycloud forest, then extract the ground and the trees, then colour the extracted tree file and
  /// apply it back onto the segmented ray cloud
  TEST(Basic, TreePaint)
  {
    EXPECT_EQ(global_command("raycreate forest 1"), 0);
    EXPECT_EQ(global_command("rayextract terrain forest.ply"), 0);
    EXPECT_EQ(global_command("rayextract trees forest.ply forest_mesh.ply --branch_segmentation"), 0);
    EXPECT_EQ(command("treecolour forest_trees.txt section_id"), 0);
    EXPECT_EQ(command("treepaint forest_trees_coloured.txt forest_segmented.ply"), 0);
    
    ray::Cloud cloud;
    EXPECT_TRUE(cloud.load("forest_segmented_painted.ply"));
    compareMoments(cloud.getMoments(), {-0.337023, 1.34537, 1.71774, 6.0926, 5.75511, 0.56438, -0.308445, 1.36047, 3.08827, 6.10555, 5.82564, 3.20507, 62.683, 36.1903, 0.328242, 0.328242, 0.328242, 1, 0.353871, 0.353871, 0.353871, 0});
  }  

  /// Create a forest then prune it
  TEST(Basic, TreePrune)
  {
    EXPECT_EQ(command("treecreate forest 7"), 0);
    EXPECT_EQ(command("treeprune forest.txt 2 cm"), 0);
    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("forest_pruned.txt"));
    compareMoments(forest.getMoments(), {20, 11.4855, 819.359, 1.53167, 0.130798, 2.6197, 0, 0, 0});
  }  

  /// Create a forest then rotate it
  TEST(Basic, TreeRotate)
  {
    EXPECT_EQ(command("treecreate forest 8"), 0);
    EXPECT_EQ(command("treerotate forest.txt 10,20,30"), 0);
    ray::ForestStructure forest;
    forest.load("forest.txt");
    compareMoments(forest.getMoments(), {20, 37.4163, 1013.45, 1.51963, 0.12723, 2.5333, 0, 0, 0});
  }

  /// Create a forest then smooth it
  TEST(Basic, TreeSmooth)
  {
    EXPECT_EQ(command("treecreate forest 9"), 0);
    EXPECT_EQ(command("treesmooth forest.txt"), 0);
    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("forest_smoothed.txt"));
    compareMoments(forest.getMoments(), {20, 16.3312, 1066.29, 1.44779, 0.114075, 2.16764, 0, 0, 0});
  }

  /// Create a forest then split it down the middle and check each side looks as it should
  TEST(Basic, TreeSplit)
  {
    EXPECT_EQ(command("treecreate forest 10"), 0);
    EXPECT_EQ(command("treesplit forest.txt plane 0.1,0.1,0.1"), 0);
    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("forest_inside.txt"));
    compareMoments(forest.getMoments(), {11, 45.1357, 426.143, 0.847207, 0.0762633, 1.67303, 0, 0, 0});
    ray::ForestStructure forest2;
    EXPECT_TRUE(forest2.load("forest_outside.txt"));
    compareMoments(forest2.getMoments(), {9, 35.2643, 372.14, 0.713625, 0.0613468, 1.22831, 0, 0, 0});
  }

  /// Create a forest and translate it
  TEST(Basic, TreeTranslate)
  {
    EXPECT_EQ(command("treecreate forest 11"), 0);
    EXPECT_EQ(command("treetranslate forest.txt 10,20,30.1"), 0);
    ray::ForestStructure forest;
    EXPECT_TRUE(forest.load("forest.txt"));
    compareMoments(forest.getMoments(), {20, 773.324, 21158.1, 1.52222, 0.129316, 2.65571, 0, 0, 0});
  }

} // raytest
