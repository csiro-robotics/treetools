// Copyright (c) 2022
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Thomas Lowe
#include "treeinformation.h"

namespace tree
{
double sqr(double x)
{
  return x * x;
}

// render the log-log data to an svg file.
void renderLogLogGraph(const std::string &filename, std::vector<Eigen::Vector2d> &loglog, double a, double b)
{
  const double width = 300;
  const double height = 200;
  const double canvas_width = width + 10;
  const double canvas_height = height + 10;
  std::ofstream ofs(filename + ".svg");
  ofs << "<svg version=\"1.1\" width=\"" << canvas_width << "\" height=\"" << canvas_height
      << "\" xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;

  double minx = std::numeric_limits<double>::max(), maxx = std::numeric_limits<double>::lowest();
  double miny = std::numeric_limits<double>::max(), maxy = std::numeric_limits<double>::lowest();
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
void calculatePowerLaw(std::vector<double> &xs, double &c, double &d, double &r2, const std::string &graph_file)
{
  std::sort(xs.begin(), xs.end());
  std::vector<Eigen::Vector2d> loglog;
  for (size_t i = 0; i < xs.size(); i++)
  {
    loglog.push_back(Eigen::Vector2d(std::log(xs[i]), std::log(static_cast<double>(xs.size() - i))));
  }
  
  std::vector<double> weights(loglog.size());
  double total_weight = std::numeric_limits<double>::min();
  for (int i = 0; i < static_cast<int>(loglog.size()); i++)
  {
    const int i0 = std::max(0, i - 1);
    const int i2 = std::min(i + 1, static_cast<int>(loglog.size()) - 1);
    weights[i] = loglog[i2][0] - loglog[i0][0];
    if (i == 0 || i == static_cast<int>(loglog.size()) - 1)
    {
      weights[i] *= 2.0;  // because it is hampered by being on the end
    }
    total_weight += weights[i];
  }
  Eigen::Vector2d mean(0, 0);
  for (size_t i = 0; i < loglog.size(); i++)
  {
    mean += weights[i] * loglog[i];
  }
  mean /= total_weight;

  double xx = std::numeric_limits<double>::min(), xy = 0.0, yy = 0.0;
  for (size_t i = 0; i < loglog.size(); i++)
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
    renderLogLogGraph(graph_file, loglog, a, b);
  }

  // convert from log-log back to power law
  c = std::exp(a);
  d = b;
}

/// @brief sets the trunk bend parameter in the tree.
/// NOTE: this relies on segment 0's radius being accurate. If we allow linear interpolation of radii then this should
/// always be tree rather than it being zero or undefined, or total radius or something
/// @param tree the tree to analyse trunk bend on
/// @param children the precalculated list of children for each segment
/// @param bend_id the id of the per-tree parameter representing the trunk bend (to fill in)
/// @param length_id the id of the parameter representing length (to fill in)
void setTrunkBend(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, int bend_id, int length_id, int branch_gradient_id)
{
  // get the trunk
  std::vector<int> ids = { 0 };
  double mean_slope = 0.0;
  double weight = 1e-10;
  for (size_t i = 0; i < ids.size(); i++)
  {
    double max_score = -1;
    int largest_child = -1;
    int id = ids[i];
    for (const auto &child : children[id])
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

      // here we estimate the secondary branch gradient
      for (const auto &child : children[id])
      {
        if (child == largest_child)
        {
          continue;
        }
        for (const auto &grandchild : children[child])
        {
          Eigen::Vector3d dif = tree.segments()[grandchild].tip - tree.segments()[child].tip;
          double w = tree.segments()[grandchild].radius;
          double h = dif[2];
          dif[2] = 0.0;
          mean_slope += w * atan2(h, dif.norm());
          weight += w;
        }
      }
    }
  }
  tree.treeAttributes()[branch_gradient_id] = mean_slope / weight;
  if (ids.size() <= 2)  // zero bend in these edge cases
  {
    return;
  }

  const double length = (tree.segments()[0].tip - tree.segments()[ids.back()].tip).norm();
  // this temporary data tructure is used to get a least squares line of best fit
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
  double total_weight = std::numeric_limits<double>::min();
  for (auto &id : ids)
  {
    auto &seg = tree.segments()[id];
    double weight = sqr(seg.radius);
    total_weight += weight;
    mean += weight * seg.tip;
  }
  mean /= total_weight;

  // code to estimate the line of best fit
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
  if (std::abs(sXX) > std::numeric_limits<double>::min())
    sXY /= sXX;

  // estimate the gradient of the line
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

  tree.treeAttributes()[bend_id] = bend;
}

/// @brief set the diameter at breast height
/// @param tree the tree to analyse
/// @param children precalculated list of children per segment
/// @param DBH_id the id of the parameter to fill in
void setDBH(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, int DBH_id)
{
  // what do we do if the tree has multiple stems?
  // I'm just going to use the average DBH
  const double breast_height = 1.3; 
  double total_DBH = 0.0;
  double num_valid_stems = 0.0;
  bool branched = false;
  double base_height = tree.segments()[0].tip[2];
  for (auto &root : children[0])
  {
    // 1. find first branch id:
    int segment = root;
    while (tree.segments()[segment].tip[2] < base_height + breast_height)
    {
      size_t num_kids = children[segment].size();
      if (num_kids == 0)
      {
        break;
      }
      else if (num_kids == 1)
      {
        segment = children[segment][0];
        branched = false;
      }
      else // pick largest child
      {
        double max_child_rad = 0.0;
        int max_child_id = 0;
        for (auto &child_id: children[segment])
        {
          double rad = tree.segments()[child_id].radius;
          if (rad >= max_child_rad)
          {
            max_child_rad = rad;
            max_child_id = child_id;
          }
        }
        segment = max_child_id;
        branched = true;
      }
    }
    if (tree.segments()[segment].tip[2] >= base_height + breast_height)
    {
      double top = tree.segments()[segment].tip[2];
      double rad = tree.segments()[segment].radius;
      int par = tree.segments()[segment].parent_id;
      if (par < 0)
      {
        std::cerr << " bad parent in setDBH" << std::endl;
        return;
      }
      double base = tree.segments()[par].tip[2];
      double rad_base = tree.segments()[par].radius;

      if (!branched) // if hasn't just branched then use linear interpolation between segments 
      {
        rad += (rad_base - rad) * (top - (base_height + breast_height))/(top - base);
      }     
      total_DBH += 2.0 * rad;
      num_valid_stems++;
    }
  }
  tree.treeAttributes()[DBH_id] = num_valid_stems > 0.0 ? total_DBH / num_valid_stems : 0.0;
}


/// How much the tree is similar to a palm tree

/// @brief analyse the tree and set the degree to which it is monocotal (palm-like in structure)
/// @param tree the tree to analyse
/// @param children precalculated list of children per segment
/// @param monocotal_id the id of the parameter to fill in representing the monocotal value
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
    for (size_t i = 0; i < list.size(); i++)
    {
      max_height = std::max(max_height, tree.segments()[list[i]].tip[2]);
      num_branches += children[list[i]].size() > 1 ? static_cast<int>(children[list[i]].size()) : 0;
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
    max_monocotal = std::max(max_monocotal, monocotal);
  }
  tree.treeAttributes()[monocotal_id] = max_monocotal;
}

void getBranchLengths(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, std::vector<double> &lengths, double prune_length)
{
  lengths.resize(tree.segments().size(), 0);
  for (size_t i = 1; i < tree.segments().size(); i++)
  {
    // for each leaf, iterate to trunk updating the maximum length...
    if (children[i].empty())  // so it is a leaf
    {
      int I = static_cast<int>(i);
      int j = tree.segments()[I].parent_id;
      lengths[I] = prune_length;
      int child = I;
      while (j != -1)
      {
        const double dist =
          lengths[child] + (tree.segments()[I].tip - tree.segments()[j].tip).norm();
        double &length = lengths[I];
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
  }
  for (auto &child : children[0])
  {
    lengths[0] = std::max(lengths[0], lengths[child]);
  }  
}

void getBifurcationProperties(ray::TreeStructure &tree, const std::vector<std::vector<int>> &children, std::vector<double> &angles, std::vector<double> &dominances, std::vector<double> &num_children, 
  double &tree_dominance, double &tree_angle, double &total_weight)
{
  angles.resize(tree.segments().size(), 0);
  dominances.resize(tree.segments().size(), 0);
  num_children.resize(tree.segments().size(), 0);
  tree_dominance = 0.0;
  tree_angle = 0.0;
  total_weight = 1e-10;
  num_children[0] = static_cast<double>(children[0].size());
  for (size_t i = 1; i < tree.segments().size(); i++)
  {
    num_children[i] = static_cast<double>(children[i].size());
    // if its a branch point then record how dominant the branching is
    if (children[i].size() > 1)
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
      double weight = sqr(max_rad) + sqr(second_max);
      const double dominance = -1.0 + 2.0 * sqr(max_rad) / weight;
      weight = std::sqrt(weight); // so we don't over bias towards values on the thick trunk
      dominances[i] = dominance;
      // now where do we spread this to?
      // if we spread to leaves then base will be empty, if we spread to parent then leave will be empty...
      tree_dominance += weight * dominance;
      total_weight += weight;

      const double branch_angle = (180.0 / ray::kPi) * std::atan2(dir1.cross(dir2).norm(), dir1.dot(dir2));
      angles[i] = branch_angle;
      tree_angle += weight * branch_angle;
    }
  }    
}
}  // namespace tree
