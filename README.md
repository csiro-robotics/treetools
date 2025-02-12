# Tree Tools
A set of command line tools for processing tree files, together with an associated C++ library.

Tree files are text files that provide a geometric description of botanical trees. At their simplest they store just the trunk location and radius per-tree, but typically they store the full Branch Structure Graph, one line per tree. A Branch Structure Graph is a piecewise cylindrical approximation of each tree, where each branch is a connected chain of cylinders or, more specifically, capsules.

The purpose of this library is to analyse and manipulate reconstructions of real trees and forests, such as from lidar-acquired points cloud reconstructions such as rayextract trees in raycloudtools (https://github.com/csiro-robotics/raycloudtools). See below for examples.

### File Format:
```console
# any initial comments
# followed by the format:
x,y,z,radius,parent_id,subtree_radius,red,green,blue,...      - the fields for each segment. Optionally this line begins with per-tree attributes e.g. 'height,crown_radius, '
comma-separated data here, with a space between sections
```
The x,y,z,radius fields are mandatory, and the parent_id field is mandatory for branch structures. All additional fields can be added at will, and are double values.

### Trunks-only example:
```console
# trunks file
x,y,z,radius
0,0,0,0.2
1,0,0,0.4
```
represents two tree trunks at positions (0,0,0) and (1,0,0), and with radii 0.2 and 0.4m respectively. 

### Branch structure example:
```console
# Full branch structure file
x,y,z,radius,parent_id,red,green,blue
0,0,0,0.2,-1,255,0,0, 0,0,1,0.1,0,255,0,0
```
represents a single tree rooted at 0,0,0 with radius 0.1m and a 1m vertical height, with colour red. The segments are separated by a space, the first has parent_id -1 which indicates that it is parentless, the subsequent segments represent cylinders from their specified x,y,z tip position to the parent's x,y,z position. The radius of this cylinder is the radius of the segment (0.1 in this case), so the root segment radius is not used in this case.

Note that there are three types of data in each line of the text file. In order: 
1. optional per-tree user-defined attributes, such as the crown radius or the tree height
2. root attributes: this is for first segment of each tree, which is parentless and volumeless. It has the same attributes as the other branch segments but the user-attributes typically represent the mean or total over the whole tree. 
3. per-branch-segment attributes such as volume or radius

## Build:
```console
git clone https://github.com/csiro-robotics/raycloudtools
```
Now follow the build instructions to build and install raycloudtools
```console
git clone treetools from this repository
mkdir build
cd build
cmake ..
make
```

To run the treeXXXX tools from anywhere, use sudo make install

## Examples:

**treecreate forest 1** 
Generate a tree file of an artifical forest using random seed 1

<p align="center">
<img img width="320" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treecreate.png?at=refs%2Fheads%2Fmaster"/>
</p>

**treeinfo forest.txt**
Output statistical information on the trees, such as total volume and number of branches. 

**treecolour forest_info.txt diameter --gradient_rgb** 
Colour the branches according to a per-branch parameter 'diameter'. Rather than greyscale, we use --gradient_rgb to colour using a red->green_blue palette. 

<p align="center">
<img img width="320" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treecolour.png?at=refs%2Fheads%2Fmaster"/>
</p>

**treemesh tree.txt**
Convert the tree file into a polygon mesh (.ply file), using the red,green,blue fields as mesh colour if available. The '-v' argument will auto-open it in meshlab if you have it installed. 

<p align="center">
<img img width="160" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treemesh.png?at=refs%2Fheads%2Fmaster"/>
</p>

**treeprune forest.txt 1 m long**
Cut the end 1 m from all branches. It is also possible to prune by branch diameter.

<p align="center">
<img img width="300" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treecreate.png?at=refs%2Fheads%2Fmaster"/>      
<img img width="300" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treeprune.png?at=refs%2Fheads%2Fmaster"/>
</p>

**treedecimate cloud_trees.txt 3 segments**
Reduce the tree to only every 3 segments, maintaining the junction points. So approximately one third as detailed.

<p align="center">
<img img width="320" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treedecimate1.png?at=refs%2Fheads%2Fmaster"/>       
<img img width="320" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treedecimate2.png?at=refs%2Fheads%2Fmaster"/>
</p>
Above: left- undecimated trees with edges rendered to show segment lengths. right - decimated to fewer segments


**treesplit cloud_trees.txt per-tree**
Split trees into separate files based on a condition, such as its radius, height or a colour. In this case we split it into one file per tree. 

<p align="center">
<img img width="640" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treesplit.png?at=refs%2Fheads%2Fmaster"/>
</p>

**treesmooth cloud_trees.txt**
Smooth the trees to have fewer sharp bends, preferentially smoothing the trunk more than the small branches. 

<p align="center">
<img img width="180" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treesmooth1.png?at=refs%2Fheads%2Fmaster"/>     
<img img width="180" src="https://raw.githubusercontent.com/csiro-robotics/treetools/master/pics/treesmooth2.png?at=refs%2Fheads%2Fmaster"/>
</p>
Above: left- reconstructed trees with excess bends. right - the trees_smoothed.txt output


**treerotate treefile.txt 0,0,30**
rotate a tree file in-place, here by 30 degrees around the z (vertical) axis

**treetranslate treefile.txt 0,0,1**
translate the tree file in-place, here by 1m in the vertical axis

**treediff forest1.txt forest2.txt**
Compare a forest to a previous version of the forest. Outputs statistics including growth rate, and the volume of woody growth and removal between the dates.

**treegrow treefile.txt 2 years**
a simple linear growth model that extends (or retracts) the branch ends, and adjusts the branch radii according to the number of years specified. 

**treefoliage treefile.txt original_raycloud.ply 0.2**
Adds a per-branch foliage_density (and foliage_sparsity) parameter according to the calculated one-sided leaf area per cubic metre within the specified distance (0.2 m) of the branch. This is used to augment branch geometry with information about how foliated each branch is.

**treepaint cloud_trees.txt cloud_segmented.ply**
When the cloud_trees.txt file is generated from rayextract trees (in raycloudtools), a cloud_segmented.ply is also generated. Using treepaint then colours the tree branch information onto the point cloud, which means it colours that leaves that each branch serves according to the branch colour in the cloud_trees.txt file. This branch colour can be set according to per-branch attributes using treecolour.

## Unit Tests

UTo build with unit tests, the CMake variable `TREE_BUILD_TESTS` must be `ON`. This can be done in the initial project configuration by running the following command from the `build` directory: `cmake  -DTREE_BUILD_TESTS=ON ..`

Unit tests may then be run by entering the build/bin folder and typing ./treetest
