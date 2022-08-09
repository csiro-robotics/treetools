// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include <raylib/raycloud.h>
#include <raylib/rayparse.h>
#include <raylib/rayrenderer.h>
#include <cstdlib>
#include <iostream>
#include "raylib/raytreegen.h"
#include "treelib/treepruner.h"
#include "treelib/treeutils.h"

double sqr(double x)
{
  return x * x;
}

void usage(int exit_code = 1)
{
  // clang-format off
  std::cout << "Bulk information for the trees, plus per-branch and per-tree information saved out." << std::endl;
  std::cout << "usage:" << std::endl;
  std::cout << "treeinfo forest.txt - report tree information and save out to _info.txt file." << std::endl;
  std::cout << std::endl;
  std::cout << "Output file fields per segment (/ on root segment):" << std::endl;
  std::cout << "  volume: volume of segment  / total tree volume" << std::endl;
  std::cout << "  diameter: diameter of segment / max diameter on tree" << std::endl;
  std::cout << "  length: length of segment base to farthest leaf" << std::endl;
  std::cout << "  strength: d^0.75/l where d is diameter of segment and l is length from segment base to leaf" << std::endl;
  std::cout << "  min_strength: minimum strength between this segment and root" << std::endl;
  std::cout << "  dominance: a1/(a1+a2) for first and second largest child branches / mean for tree" << std::endl;
  std::cout << "  angle: angle between branches at each branch point / mean branch angle" << std::endl;
  std::cout << "  bend: bend of main trunk (standard deviation from straight line / length)" << std::endl;
  std::cout << "  children: number of children per branch / mean for tree" << std::endl;
  std::cout << "Use treecolour 'field' to colour per-segment or treecolour trunk 'field' to colour per tree from root segment." << std::endl;
  std::cout << "Then use treemesh to render based on this colour output." << std::endl;
  // clang-format on
  exit(exit_code);
}

// render the log-log data to an svg file.
void renderGraph(const std::string &filename, std::vector<Eigen::Vector2d> &loglog, double a, double b)
{
  const double width = 300;
  const double height = 200;
  const double canvas_width = width + 10;
  const double canvas_height = height + 10;
  std::ofstream ofs(filename + ".svg");
  ofs << "<svg version=\"1.1\" width=\"" << canvas_width << "\" height=\"" << canvas_height
      << "\" xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;

  double minx = 1e10, maxx = -1e10;
  double miny = 1e10, maxy = -1e10;
  for (auto &p : loglog)
  {
    minx = std::min(minx, p[0]);
    maxx = std::max(maxx, p[0]);
    miny = std::min(miny, p[1]);
    maxy = std::max(maxy, p[1]);
  }
  const Eigen::Vector2d horiz0(0, 0), horiz1(width, 0);
  const Eigen::Vector2d vert0(0, 0), vert1(0, height);
  const Eigen::Vector2d bf0(0, height * ((a + minx * b) - miny) / (maxy - miny)),
    bf1(width, height * ((a + maxx * b) - miny) / (maxy - miny));
  const std::vector<Eigen::Vector2d> v0 = { horiz0, vert0, bf0 };
  const std::vector<Eigen::Vector2d> v1 = { horiz1, vert1, bf1 };
  for (size_t i = 0; i < v0.size(); i++)
  {
    ofs << "<line x1=\"" << v0[i][0] << "\" y1=\"" << canvas_height - v0[i][1] << "\" x2=\"" << v1[i][0] << "\" y2=\""
        << canvas_height - v1[i][1] << "\" style=\"stroke:rgb(0,0,0);stroke-width:1\" />" << std::endl;
  }

  for (auto &p : loglog)
  {
    const double x = width * (p[0] - minx) / (maxx - minx);
    const double y = height * (p[1] - miny) / (maxy - miny);
    const double rad = 1;
    ofs << "<circle cx=\"" << x << "\" cy=\"" << canvas_height - y << "\" r=\"" << rad
        << "\" stroke-width=\"0\" fill=\"green\" />" << std::endl;
  }

  ofs << "<text x=\"" << width / 2.0 << "\" y=\"" << canvas_height - 3
      << "\" font-size=\"8\" text-anchor=\"middle\" fill=\"black\">log " << filename << "</text>" << std::endl;
  ofs << "<text font-size=\"8\" text-anchor=\"middle\" fill=\"black\" transform=\"translate(" << 8 << ","
      << canvas_height / 2.0 << ") rotate(-90)\">log number larger</text>" << std::endl;
  ofs << "</svg>" << std::endl;
}

// calculates the power law: # data larger than x = cx^d, with correlation coefficient r2
void calculatePowerLaw(std::vector<double> &xs, double &c, double &d, double &r2, const std::string &graph_file = "")
{
  std::sort(xs.begin(), xs.end());
  std::vector<Eigen::Vector2d> loglog;
  for (int i = 0; i < (int)xs.size(); i++)
  {
    loglog.push_back(Eigen::Vector2d(std::log(xs[i]), std::log((double)(xs.size() - i))));
  }
  std::vector<double> weights(loglog.size());
  double total_weight = 1e-10;
  for (int i = 0; i < (int)loglog.size(); i++)
  {
    const int i0 = std::max(0, i - 1);
    const int i2 = std::min(i + 1, (int)loglog.size() - 1);
    weights[i] = loglog[i2][0] - loglog[i0][0];
    if (i == 0 || i == (int)loglog.size() - 1)
    {
      weights[i] *= 2.0;  // because it is hampered by being on the end
    }
    total_weight += weights[i];
  }
  Eigen::Vector2d mean(0, 0);
  for (int i = 0; i < (int)loglog.size(); i++)
  {
    mean += weights[i] * loglog[i];
  }
  mean /= total_weight;

  double xx = 1e-10, xy = 0.0, yy = 0.0;
  for (int i = 0; i < (int)loglog.size(); i++)
  {
    Eigen::Vector2d p = loglog[i] - mean;
    xx += weights[i] * p[0] * p[0];
    xy += weights[i] * p[0] * p[1];
    yy += weights[i] * p[1] * p[1];
  }

  // based on http://mathworld.wolfram.com/LeastSquaresFitting.html
  // log # = a + b * log diam
  const double b = xy / xx;
  const double a = mean[1] - b * mean[0];
  r2 = xy * xy / (xx * yy);
  if (graph_file != "")
  {
    renderGraph(graph_file, loglog, a, b);
  }

  // convert from log-log back to power law
  c = std::exp(a);
  d = b;
}

// TODO: this relies on segment 0's radius being accurate. If we allow linear interpolation of radii then this should
// always be tree rather than it being zero or undefined, or total radius or something
void setTrunkBend(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, int bend_id, int length_id)
{
  // get the trunk
  std::vector<int> ids = { 0 };
  for (size_t i = 0; i < ids.size(); i++)
  {
    double max_score = -1;
    int largest_child = -1;
    for (const auto &child : children[ids[i]])
    {
      // we pick the route which has the longer and wider branch
      double score = tree.segments()[child].radius * tree.segments()[child].attributes[length_id];
      if (score > max_score)
      {
        max_score = score;
        largest_child = child;
      }
    }
    if (largest_child != -1)
    {
      ids.push_back(largest_child);
    }
  }
  if (ids.size() <= 2)  // zero bend in these edge cases
  {
    return;
  }

  const double length = (tree.segments()[0].tip - tree.segments()[ids.back()].tip).norm();
  struct Accumulator
  {
    Accumulator()
      : x(0)
      , y(0, 0)
      , xy(0, 0)
      , x2(0)
    {}
    double x;
    Eigen::Vector2d y;
    Eigen::Vector2d xy;
    double x2;
  };
  Accumulator sum;
  Eigen::Vector3d mean(0, 0, 0);
  double total_weight = 1e-10;
  for (auto &id : ids)
  {
    auto &seg = tree.segments()[id];
    double weight = sqr(seg.radius);
    total_weight += weight;
    mean += weight * seg.tip;
  }
  mean /= total_weight;

  for (auto &id : ids)
  {
    auto &seg = tree.segments()[id];
    const Eigen::Vector3d to_point = seg.tip - mean;
    const Eigen::Vector2d offset(to_point[0], to_point[1]);
    const double w = sqr(seg.radius);
    const double h = to_point[2];
    sum.x += h * w;
    sum.y += offset * w;
    sum.xy += h * offset * w;
    sum.x2 += h * h * w;
  }

  // based on http://mathworld.wolfram.com/LeastSquaresFitting.html
  Eigen::Vector2d sXY = sum.xy - sum.x * sum.y / total_weight;
  const double sXX = sum.x2 - sum.x * sum.x / total_weight;
  if (std::abs(sXX) > 1e-10)
    sXY /= sXX;

  const Eigen::Vector3d grad(sXY[0], sXY[1], 1.0);

  // now get sigma relative to the line
  double variance = 0.0;
  for (auto &id : ids)
  {
    auto &seg = tree.segments()[id];
    const double h = seg.tip[2] - mean[2];
    const Eigen::Vector3d pos = mean + grad * h;
    Eigen::Vector3d dif = (pos - seg.tip);
    dif[2] = 0.0;
    variance += dif.squaredNorm() * sqr(seg.radius);
  }
  variance /= total_weight;
  const double sigma = std::sqrt(variance);
  const double bend = sigma / length;

  for (auto &id : ids)
  {
    tree.segments()[id].attributes[bend_id] = bend;
  }
}

/// How much the tree is similar to a palm tree
void setMonocotal(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, int monocotal_id)
{
  // One per child of root, this is because many palms can grow from a single point at the bottom.
  double max_monocotal = 0.0;
  for (auto &root : children[0])
  {
    // 1. find first branch id:
    int segment = root;
    while (children[segment].size() == 1)
    {
      segment = children[segment][0];
    }
    // 2. get distance to root:
    const Eigen::Vector3d branch_point = tree.segments()[segment].tip;
    const double straight_distance = (branch_point - tree.segments()[0].tip).norm();
    // 3. get path length to root:
    double path_length = 0.0;
    int top_segment = segment;
    while (tree.segments()[segment].parent_id != -1)
    {
      const int par = tree.segments()[segment].parent_id;
      path_length += (tree.segments()[segment].tip - tree.segments()[par].tip).norm();
      segment = par;
    }
    // 4. get height difference to top of tree
    std::vector<int> list = { root };
    double max_height = tree.segments()[top_segment].tip[2];
    int num_branches = 0;
    for (int i = 0; i < (int)list.size(); i++)
    {
      max_height = std::max(max_height, tree.segments()[list[i]].tip[2]);
      num_branches += children[list[i]].size() > 1 ? (int)children[list[i]].size() : 0;
      list.insert(list.end(), children[list[i]].begin(), children[list[i]].end());
    }
    const double dist_to_top = max_height - tree.segments()[top_segment].tip[2];

    // 5. combine into a value for 'palmtree-ness'
    // this rewards a straight trunk with a small height from first branch point to peak
    double monocotal = straight_distance / (path_length + dist_to_top);  
    if (num_branches < 5)  // this is a long pole with little on top. We shouldn't consider this a signal of being
                           // monocotal, even though it could be a dead one
    {
      monocotal = 0.0;
    }

    // 6. fill in the attributes for this root upwards
    for (auto &id : list)
    {
      tree.segments()[id].attributes[monocotal_id] = monocotal;
    }
    max_monocotal = std::max(max_monocotal, monocotal);
  }
  tree.segments()[0].attributes[monocotal_id] = max_monocotal;
}

int main(int argc, char *argv[])
{
  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout.precision(3);
  ray::FileArgument forest_file;
  const bool parsed = ray::parseCommandLine(argc, argv, { &forest_file });
  if (!parsed)
  {
    usage();
  }

  ray::ForestStructure forest;
  if (!forest.load(forest_file.name()))
  {
    usage();
  }
  if (forest.trees.size() != 0 && forest.trees[0].segments().size() == 0)
  {
    std::cout << "info only works on tree structures, not trunks-only files" << std::endl;
    usage();
  }
  // attributes to estimate
  // 5. fractal distribution of trunk diameters?
  // 8. fractal distribution of branch diameters?
  std::cout << "Information" << std::endl;
  std::cout << std::endl;

  const int num_attributes = (int)forest.trees[0].attributes().size();
  const std::vector<std::string> new_attributes = { "volume",       "diameter",  "length",   "strength",
                                                    "min_strength", "dominance", "angle",    "bend",
                                                    "children",     "dimension", "monocotal" };
  const int volume_id = num_attributes + 0;
  const int diameter_id = num_attributes + 1;
  const int length_id = num_attributes + 2;
  const int strength_id = num_attributes + 3;
  const int min_strength_id = num_attributes + 4;
  const int dominance_id = num_attributes + 5;
  const int angle_id = num_attributes + 6;
  const int bend_id = num_attributes + 7;
  const int children_id = num_attributes + 8;
  const int dimension_id = num_attributes + 9;
  const int monocotal_id = num_attributes + 10;
  auto &att = forest.trees[0].attributes();
  for (auto &new_at : new_attributes)
  {
    if (std::find(att.begin(), att.end(), new_at) != att.end())
    {
      std::cerr << "Error: cannot add info that is already present: " << new_at << std::endl;
      usage();
    }
  }

  std::cout << "Additional attributes mean, min, max:" << std::endl;
  for (int i = 0; i < num_attributes; i++)
  {
    std::cout << "\t" << att[i] << ":";
    for (int j = 0; j < 30 - (int)att[i].length(); j++) std::cout << " ";
    double value = 0.0;
    double num = 0.0;
    double max_val = -1e10;
    double min_val = 1e10;
    for (auto &tree : forest.trees)
    {
      for (auto &segment : tree.segments())
      {
        const double val = segment.attributes[i];
        max_val = std::max(max_val, val);
        min_val = std::min(min_val, val);
        value += val;
        num++;
      }
    }
    std::cout << value / num << ",\t" << min_val << ",\t" << max_val << std::endl;
  }
  std::cout << std::endl;

  // Fill in blank attributes across the whole structure
  double min_branch_radius = 1e10;
  double max_branch_radius = -1e10;
  double total_branch_radius = 0.0;
  int num_total = 0;
  for (auto &tree : forest.trees)
  {
    for (auto &new_at : new_attributes) 
    {
      tree.attributes().push_back(new_at);
    }
    for (auto &segment : tree.segments())
    {
      min_branch_radius = std::min(min_branch_radius, segment.radius);
      max_branch_radius = std::max(max_branch_radius, segment.radius);
      total_branch_radius += segment.radius;
      num_total++;
      for (size_t i = 0; i < new_attributes.size(); i++)
      {
        segment.attributes.push_back(0);
      }
    }
  }
  const double broken_diameter = 0.6;  // larger than this is considered a broken branch and not extended
  const double taper_ratio = 140.0;

  double total_volume = 0.0;
  double min_volume = 1e10, max_volume = -1e10;
  double total_diameter = 0.0;
  double min_diameter = 1e10, max_diameter = -1e10;
  double total_height = 0.0;
  double min_height = 1e10, max_height = -1e10;
  double total_strength = 0.0;
  double min_strength = 1e10, max_strength = -1e10;
  double total_dominance = 0.0;
  double min_dominance = 1e10, max_dominance = -1e10;
  double total_angle = 0.0;
  double min_angle = 1e10, max_angle = -1e10;
  double total_bend = 0.0;
  double min_bend = 1e10, max_bend = -1e10;
  double total_children = 0.0;
  double min_children = 1e10, max_children = -1e10;
  double total_dimension = 0.0;
  double min_dimension = 1e10, max_dimension = -1e10;
  int num_stat_trees = 0;  // used for dimension values

  int num_branched_trees = 0;
  std::vector<double> tree_lengths;
  for (auto &tree : forest.trees)
  {
    // get the children
    std::vector<std::vector<int>> children(tree.segments().size());
    for (size_t i = 1; i < tree.segments().size(); i++) 
    {
      children[tree.segments()[i].parent_id].push_back((int)i);
    }

    double tree_dominance = 0.0;
    double tree_angle = 0.0;
    double total_weight = 0.0;
    double tree_height = 0;
    double tree_children = 0.0;
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      tree_height = std::max(tree_height, tree.segments()[i].tip[2] - tree.segments()[0].tip[2]);
      // for each leaf, iterate to trunk updating the maximum length...
      if (children[i].empty())  // so it is a leaf
      {
        const double extension =
          2.0 * tree.segments()[i].radius > broken_diameter ? 0.0 : (taper_ratio * tree.segments()[i].radius);
        int I = (int)i;
        int j = tree.segments()[I].parent_id;
        tree.segments()[I].attributes[length_id] = extension;
        int child = I;
        while (j != -1)
        {
          const double dist =
            tree.segments()[child].attributes[length_id] + (tree.segments()[I].tip - tree.segments()[j].tip).norm();
          double &length = tree.segments()[I].attributes[length_id];
          if (dist > length)
          {
            length = dist;
          }
          else
          {
            break;
          }
          child = I;
          I = j;
          j = tree.segments()[I].parent_id;
        }
      }
      // if its a branch point then record how dominant the branching is
      else if (children[i].size() > 1)
      {
        double max_rad = -1.0;
        double second_max = -1.0;
        Eigen::Vector3d dir1(0, 0, 0), dir2(0, 0, 0);
        for (auto &child : children[i])
        {
          double rad = tree.segments()[child].radius;
          Eigen::Vector3d dir = tree.segments()[child].tip - tree.segments()[i].tip;
          // we go up a segment if we can, as the radius and angle will have settled better here
          if (children[child].size() == 1)  
          {
            rad = tree.segments()[children[child][0]].radius;
            dir = tree.segments()[children[child][0]].tip - tree.segments()[child].tip;
          }
          if (rad > max_rad)
          {
            second_max = max_rad;
            max_rad = rad;
            dir2 = dir1;
            dir1 = dir;
          }
          else if (rad > second_max)
          {
            second_max = rad;
            dir2 = dir;
          }
        }
        const double weight = sqr(max_rad) + sqr(second_max);
        const double dominance = -1.0 + 2.0 * sqr(max_rad) / weight;
        tree.segments()[i].attributes[dominance_id] = dominance;
        // now where do we spread this to?
        // if we spread to leaves then base will be empty, if we spread to parent then leave will be empty...
        tree_dominance += weight * dominance;
        total_weight += weight;
        const double branch_angle = (180.0 / ray::kPi) * std::atan2(dir1.cross(dir2).norm(), dir1.dot(dir2));
        tree.segments()[i].attributes[angle_id] = branch_angle;
        tree_angle += weight * branch_angle;
        tree.segments()[i].attributes[children_id] = (double)children[i].size();
        tree_children += weight * (double)children[i].size();
      }
    }
    for (auto &child : children[0])
    {
      tree.segments()[0].attributes[length_id] =
        std::max(tree.segments()[0].attributes[length_id], tree.segments()[child].attributes[length_id]);
    }
    if (children[0].size() > 0)
    {
      tree.segments()[0].attributes[children_id] = (double)children[0].size();
    }
    setTrunkBend(tree, children, bend_id, length_id);
    setMonocotal(tree, children, monocotal_id);

    std::vector<double> lengths;
    for (auto &seg : tree.segments())
    {
      if (seg.attributes[children_id] > 1)
      {
        lengths.push_back(seg.attributes[length_id]);
      }
    }
    const int min_branch_count = 6;  // can't ddo any reasonable stats with fewer than this number of branches
    if (lengths.size() >= min_branch_count)
    {
      double c, d, r2;
      calculatePowerLaw(lengths, c, d, r2);
      const double tree_dimension = std::min(-d, 3.0);
      for (auto &seg : tree.segments()) 
      {
        seg.attributes[dimension_id] = tree_dimension;
      }
      total_dimension += tree_dimension;
      max_dimension = std::max(max_dimension, tree_dimension);
      min_dimension = std::min(min_dimension, tree_dimension);
      num_stat_trees++;
    }

    if (total_weight > 0.0)
    {
      tree_dominance /= total_weight;
      tree_angle /= total_weight;
      num_branched_trees++;
      total_dominance += tree_dominance;
      min_dominance = std::min(min_dominance, tree_dominance);
      max_dominance = std::max(max_dominance, tree_dominance);
      total_angle += tree_angle;
      min_angle = std::min(min_angle, tree_angle);
      max_angle = std::max(max_angle, tree_angle);
      tree_children /= total_weight;
      total_children += tree_children;
      min_children = std::min(min_children, tree_children);
      max_children = std::max(max_children, tree_children);
    }
    tree.segments()[0].attributes[dominance_id] = tree_dominance;
    tree.segments()[0].attributes[angle_id] = tree_angle;
    tree.segments()[0].attributes[children_id] = tree_children;

    double tree_volume = 0.0;
    double tree_diameter = 0.0;
    for (size_t i = 1; i < tree.segments().size(); i++)
    {
      auto &branch = tree.segments()[i];
      const double volume =
        ray::kPi * (branch.tip - tree.segments()[branch.parent_id].tip).norm() * branch.radius * branch.radius;
      branch.attributes[volume_id] = volume;
      branch.attributes[diameter_id] = 2.0 * branch.radius;
      tree_diameter = std::max(tree_diameter, branch.attributes[diameter_id]);
      tree_volume += volume;
      const double denom = std::max(1e-10, branch.attributes[length_id]);  // avoid a divide by 0
      branch.attributes[strength_id] = std::pow(branch.attributes[diameter_id], 3.0 / 4.0) / denom;
    }
    tree.segments()[0].attributes[volume_id] = tree_volume;
    total_volume += tree_volume;
    min_volume = std::min(min_volume, tree_volume);
    max_volume = std::max(max_volume, tree_volume);
    tree.segments()[0].attributes[diameter_id] = tree_diameter;
    total_diameter += tree_diameter;
    min_diameter = std::min(min_diameter, tree_diameter);
    max_diameter = std::max(max_diameter, tree_diameter);
    total_height += tree_height;
    min_height = std::min(min_height, tree_height);
    max_height = std::max(max_height, tree_height);
    tree_lengths.push_back(tree.segments()[0].attributes[length_id]);
    tree.segments()[0].attributes[strength_id] =
      std::pow(tree_diameter, 3.0 / 4.0) / std::max(1e-10, tree.segments()[0].attributes[length_id]);
    const double tree_strength = tree.segments()[0].attributes[strength_id];
    total_strength += tree_strength;
    min_strength = std::min(min_strength, tree_strength);
    max_strength = std::max(max_strength, tree_strength);
    const double tree_bend = tree.segments()[0].attributes[bend_id];
    total_bend += tree_bend;
    min_bend = std::min(min_bend, tree_bend);
    max_bend = std::max(max_bend, tree_bend);

    // alright, now how do we get the minimum strength from tip to root?
    for (auto &segment : tree.segments()) 
    {
      segment.attributes[min_strength_id] = 1e10;
    }
    std::vector<int> inds = children[0];
    for (size_t i = 0; i < inds.size(); i++)
    {
      int j = inds[i];
      auto &seg = tree.segments()[j];
      seg.attributes[min_strength_id] =
        std::min(seg.attributes[strength_id], tree.segments()[seg.parent_id].attributes[min_strength_id]);
      for (auto &child : children[j]) 
      {
        inds.push_back(child);
      }
    }
    tree.segments()[0].attributes[min_strength_id] = tree.segments()[0].attributes[strength_id];  // no different
  }
  std::cout << "Number of:" << std::endl;
  std::cout << "                  trees: " << forest.trees.size() << std::endl;

  // Trunk power laws:
  std::vector<double> diameters;
  for (auto &tree : forest.trees)
  {
    diameters.push_back(2.0 * tree.segments()[0].radius);
  }
  double c, d, r2;
  calculatePowerLaw(diameters, c, d, r2, "trunkwidth");
  std::cout << "    trunks wider than x: " << c << "x^" << d << "\t\twith correlation (r2) " << r2 << std::endl;
  calculatePowerLaw(tree_lengths, c, d, r2, "treelength");
  std::cout << "    trees longer than l: " << c << "l^" << d << "\twith correlation (r2) " << r2 << std::endl;

  // Branch power laws:
  std::vector<double> lengths;
  for (auto &tree : forest.trees)
  {
    for (auto &seg : tree.segments())
    {
      if (seg.attributes[children_id] > 1.0)
        lengths.push_back(seg.attributes[length_id]);
    }
  }
  calculatePowerLaw(lengths, c, d, r2, "branchlength");
  std::cout << " branches longer than l: " << c << "l^" << d << "\twith correlation (r2) " << r2 << std::endl;
  std::cout << std::endl;

  std::cout << "Total:" << std::endl;
  std::cout << "              volume of wood: " << total_volume
            << " m^3.\tMean,min,max: " << total_volume / (double)forest.trees.size() << ", " << min_volume << ", "
            << max_volume << " m^3" << std::endl;
  std::cout << " mass of wood (at 0.5 T/m^3): " << 0.5 * total_volume
            << " Tonnes.\tMean,min,max: " << 500.0 * total_volume / (double)forest.trees.size() << ", "
            << 500.0 * min_volume << ", " << 500.0 * max_volume << " kg" << std::endl;
  std::cout << std::endl;

  std::cout << "Per-tree mean, min, max:" << std::endl;
  std::cout << "                trunk diameter (m): " << total_diameter / (double)forest.trees.size() << ",\t"
            << min_diameter << ",\t" << max_diameter << std::endl;
  std::cout << "                   tree height (m): " << total_height / (double)forest.trees.size() << ",\t"
            << min_height << ",\t" << max_height << std::endl;
  std::cout << " trunk strength (diam^0.75/length): " << total_strength / (double)forest.trees.size() << ",\t"
            << min_strength << ",\t" << max_strength << std::endl;
  std::cout << "         branch dominance (0 to 1): " << total_dominance / (double)num_branched_trees << ",\t"
            << min_dominance << ",\t" << max_dominance << std::endl;
  std::cout << "            branch angle (degrees): " << total_angle / (double)num_branched_trees << ",\t" << min_angle
            << ",\t" << max_angle << std::endl;
  std::cout << "              trunk bend (degrees): " << total_bend / (double)forest.trees.size() << ",\t" << min_bend
            << ",\t" << max_bend << std::endl;
  std::cout << "               children per branch: " << total_children / (double)num_branched_trees << ",\t"
            << min_children << ",\t" << max_children << std::endl;
  std::cout << "          dimension (w.r.t length): " << total_dimension / (double)num_stat_trees << ",\t"
            << min_dimension << ",\t" << max_dimension << std::endl;
  std::cout << std::endl;

  std::cout << "Per-branch mean, min, max:" << std::endl;
  std::cout << "                     diameter (cm): " << 200.0 * total_branch_radius / (double)num_total << ",\t"
            << 200.0 * min_branch_radius << ",\t" << 200.0 * max_branch_radius << std::endl;
  std::cout << std::endl;

  std::cout << "saving per-tree and per-segment data to file" << std::endl;
  forest.save(forest_file.nameStub() + "_info.txt");
  return 1;
}
