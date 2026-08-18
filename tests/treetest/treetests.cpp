// Copyright (c) 2020
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe

#include <cstdlib>
#include <vector>

#include "gtest/gtest.h"

#include "treelib/treelibconfig.h"

#include "raylib/raycloud.h"
#include "raylib/rayforeststructure.h"
#include "raylib/raymesh.h"
#include "raylib/rayply.h"

#include "environment.h"

/// Tree tools testing framework. In each test, the statistics of the resulting
/// clouds are compared to the statistics of the tree file when it was confirmed
/// to be operating correctly.
namespace treetest
{
/// Issues the specified system command, including the required prefix on non-windows systems.
int command(const std::string &system_command)
{
  return treetest::Environment::Command(system_command);
}

/// Issues the command to copy a file, which is a platform dependent system command.
int copy(const std::string &copy_command)
{
#ifdef _WIN32
  return system("copy " + copy_command);
#else
  return system(("cp " + copy_command).c_str());
#endif  // _WIN32
}

/// Compare the statistical (1st and 2nd order) moments of the two ray clouds.
/// This almost surely detects differing clouds, and always equal clouds, given
/// a tolerance @c eps .
size_t compareMoments(const Eigen::ArrayXd &m1, const std::vector<double> &m2, double eps = 0.1)
{
  size_t i;
  for (i = 0; i < m2.size(); i++)
  {
    EXPECT_GT(m1[i], m2[i] - eps);
    EXPECT_LT(m1[i], m2[i] + eps);
    if (std::abs(m1[i] - m2[i]) > eps)
    {
      return i;
    }
  }
  return i;
}

/// Colour a tree according to the branch lengths
TEST(Basic, TreeColour)
{
  EXPECT_EQ(command("treecreate forest 1"), 0);
  EXPECT_EQ(command("treeinfo forest.txt"), 0);
  EXPECT_EQ(command("treecolour forest_info.txt length"), 0);
  ray::ForestStructure forest;
  EXPECT_TRUE(forest.load("forest_info_coloured.txt"));
  EXPECT_EQ(compareMoments(forest.getMoments(),
                           { 20.0, 25.8189, 908.824, 1.53544, 0.129849, 2.59429, 88066.0, 14.45297, 1.18585 }),
            forest.getMoments().size());
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
  EXPECT_EQ(compareMoments(forest.getMoments(), { 2, 0, 0, 0.214118, 0.0229267, 0.574374, 0, 0, 0 }),
            forest.getMoments().size());
}

/// Create a tree and create a forest
TEST(Basic, TreeCreate)
{
  EXPECT_EQ(command("treecreate tree 3"), 0);
  ray::ForestStructure forest;
  forest.load("tree.txt");
  EXPECT_EQ(compareMoments(forest.getMoments(), { 1, 0, 0, 0.104859, 0.0109953, 0.276382, 0, 0, 0 }),
            forest.getMoments().size());

  EXPECT_EQ(command("treecreate forest 2"), 0);
  ray::ForestStructure forest2;
  EXPECT_TRUE(forest2.load("forest.txt"));
  EXPECT_EQ(compareMoments(forest2.getMoments(), { 20, 34.3553, 1061.61, 1.51633, 0.128301, 2.60812, 0, 0, 0 }),
            forest2.getMoments().size());
}

/// Create a forest, then decimate
TEST(Basic, TreeDecimate)
{
  EXPECT_EQ(command("treecreate forest 3"), 0);
  EXPECT_EQ(command("treedecimate forest.txt 3 segments"), 0);
  ray::ForestStructure forest;
  EXPECT_TRUE(forest.load("forest_decimated.txt"));
  EXPECT_EQ(compareMoments(forest.getMoments(), { 20, 31.5473, 974.846, 1.52272, 0.129441, 2.66069, 0, 0, 0 }),
            forest.getMoments().size());
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
  EXPECT_EQ(command("raycreate forest 14"), 0);
  EXPECT_EQ(command("rayextract terrain forest.ply"), 0);
  EXPECT_EQ(command("rayextract trees forest.ply forest_mesh.ply"), 0);
  EXPECT_EQ(command("treefoliage forest_trees.txt forest.ply 0.3"), 0);

  ray::ForestStructure forest;
  EXPECT_TRUE(forest.load("forest_trees_foliage.txt"));
  EXPECT_EQ(
    compareMoments(forest.getMoments(), { 20, 22.65207, 1037.4448, 1.51422, 0.128055, 2.41613, 86012, 0, 109.189364 }),
    forest.getMoments().size());
  ray::Cloud cloud;
  EXPECT_TRUE(cloud.load("forest_densities.ply"));
  EXPECT_EQ(compareMoments(cloud.getMoments(),
                           { 0.273292,  0.432565, 1.74041,  5.61377,  5.67701,  0.621977, 0.3418376, 0.4062955,
                             3.09509,   5.66319,  5.69592,  3.16601,  63.8625,  36.8713,  0.0947165, 0.0947165,
                             0.0947165, 1,        0.158243, 0.158243, 0.158243, 0 }),
            cloud.getMoments().size());
}

/// Create a forest, then grow it
TEST(Basic, TreeGrow)
{
  EXPECT_EQ(command("treecreate forest 5"), 0);
  EXPECT_EQ(command("treegrow forest.txt 3 years"), 0);
  ray::ForestStructure forest;
  forest.load("forest_grown.txt");
  EXPECT_EQ(compareMoments(forest.getMoments(), { 20, 46.4312, 681.105, 1.61261, 0.142173, 5.009566, 0, 0, 0 }),
            forest.getMoments().size());

  EXPECT_EQ(command("treegrow forest.txt -2 years"), 0);
  ray::ForestStructure forest2;
  EXPECT_TRUE(forest2.load("forest_grown.txt"));
  EXPECT_EQ(compareMoments(forest2.getMoments(), { 20, 46.4312, 681.105, 1.41261, 0.111921, 1.80229, 0, 0, 0 }),
            forest2.getMoments().size());
}

/// Create a forest then get info on it
TEST(Basic, TreeInfo)
{
  EXPECT_EQ(command("treecreate forest 6"), 0);
  EXPECT_EQ(command("treeinfo forest.txt"), 0);
  ray::ForestStructure forest;
  EXPECT_TRUE(forest.load("forest_info.txt"));
  EXPECT_EQ(
    compareMoments(forest.getMoments(), { 20.000, 22.294, 944.819, 1.534, 0.136, 2.909, 39411, 14.385680, 0.956 }),
    forest.getMoments().size());
}

/// Create a forest then mesh it
TEST(Basic, TreeMesh)
{
  EXPECT_EQ(command("treecreate forest 13"), 0);
  EXPECT_EQ(command("treemesh forest.txt"), 0);
  ray::Mesh mesh;
  EXPECT_TRUE(ray::readPlyMesh("forest_mesh.ply", mesh));
  EXPECT_EQ(compareMoments(mesh.getMoments(), { 0.985852, -0.201182, 5.18758, 6.22901, 6.14931, 2.09894 }),
            mesh.getMoments().size());
}

/// Create a raycloud forest, then extract the ground and the trees, then colour the extracted tree file and
/// apply it back onto the segmented ray cloud
TEST(Basic, TreePaint)
{
  EXPECT_EQ(command("raycreate forest 1"), 0);
  EXPECT_EQ(command("rayextract terrain forest.ply"), 0);
  EXPECT_EQ(command("rayextract trees forest.ply forest_mesh.ply --branch_segmentation"), 0);
  EXPECT_EQ(command("treecolour forest_trees.txt section_id"), 0);
  EXPECT_EQ(command("treepaint forest_trees_coloured.txt forest_segmented.ply"), 0);

  ray::Cloud cloud;
  EXPECT_TRUE(cloud.load("forest_segmented_painted.ply"));
  EXPECT_EQ(compareMoments(cloud.getMoments(),
                           { -0.337023, 1.345368, 1.717670, 6.092560, 5.755113, 0.564411, -0.308445, 1.360467,
                             3.088200,  6.105547, 5.825643, 3.20514,  62.683,   36.1903,  0.324531,  0.324531,
                             0.324531,  1.00000,  0.359649, 0.359649, 0.359649, 0 }),
            cloud.getMoments().size());
}

/// Create a forest then prune it
TEST(Basic, TreePrune)
{
  EXPECT_EQ(command("treecreate forest 7"), 0);
  EXPECT_EQ(command("treeprune forest.txt 2 cm"), 0);
  ray::ForestStructure forest;
  EXPECT_TRUE(forest.load("forest_pruned.txt"));
  EXPECT_EQ(compareMoments(forest.getMoments(), { 20, 11.4855, 819.359, 1.53167, 0.130798, 2.6197, 0, 0, 0 }),
            forest.getMoments().size());
}

/// Create a forest then rotate it
TEST(Basic, TreeRotate)
{
  EXPECT_EQ(command("treecreate forest 8"), 0);
  EXPECT_EQ(command("treerotate forest.txt 10,20,30"), 0);
  ray::ForestStructure forest;
  forest.load("forest.txt");
  EXPECT_EQ(compareMoments(forest.getMoments(), { 20, 37.4163, 1013.448630, 1.51963, 0.12723, 2.5333, 0, 0, 0 }),
            forest.getMoments().size());
}

/// Create a forest then smooth it
TEST(Basic, TreeSmooth)
{
  EXPECT_EQ(command("treecreate forest 9"), 0);
  EXPECT_EQ(command("treesmooth forest.txt"), 0);
  ray::ForestStructure forest;
  EXPECT_TRUE(forest.load("forest_smoothed.txt"));
  EXPECT_EQ(compareMoments(forest.getMoments(), { 20, 16.3312, 1066.29, 1.44779, 0.114075, 2.16764, 0, 0, 0 }),
            forest.getMoments().size());
}

/// Create a forest then split it down the middle and check each side looks as it should
TEST(Basic, TreeSplit)
{
  EXPECT_EQ(command("treecreate forest 10"), 0);
  EXPECT_EQ(command("treesplit forest.txt plane 0.1,0.1,0.1"), 0);
  ray::ForestStructure forest;
  EXPECT_TRUE(forest.load("forest_inside.txt"));
  EXPECT_EQ(compareMoments(forest.getMoments(), { 11, 45.1357, 426.143, 0.847207, 0.0762633, 1.67303, 0, 0, 0 }),
            forest.getMoments().size());
  ray::ForestStructure forest2;
  EXPECT_TRUE(forest2.load("forest_outside.txt"));
  EXPECT_EQ(compareMoments(forest2.getMoments(), { 9, 35.2643, 372.14, 0.713625, 0.0613468, 1.22831, 0, 0, 0 }),
            forest2.getMoments().size());
}

/// Create a forest and translate it
TEST(Basic, TreeTranslate)
{
  EXPECT_EQ(command("treecreate forest 11"), 0);
  EXPECT_EQ(command("treetranslate forest.txt 10,20,30.1"), 0);
  ray::ForestStructure forest;
  EXPECT_TRUE(forest.load("forest.txt"));
  EXPECT_EQ(compareMoments(forest.getMoments(), { 20, 773.323755, 21158.142314, 1.52222, 0.129316, 2.65571, 0, 0, 0 }),
            forest.getMoments().size());
}
}  // namespace treetest
