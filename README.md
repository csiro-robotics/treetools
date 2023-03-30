# Tree Tools
A set of command line tools for processing tree files, together with an associated C++ library.

Tree files are text files that can either store a tree trunk per tree, or a full Branch Structure Graph (connected cylinders) per tree. One line per tree.

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
cd raycloudtools
git checkout experimental
mkdir build
cd build
cmake ..
make
sudo make install
cd ../..
git clone treetools from this repository
mkdir build
cd build
cmake ..
make
```

To run the treeXXXX tools from anywhere, use sudo make install, or place in your ~/bashrc:
```console
  export PATH=$PATH:'source code path'/treetools/build/bin
```


## Examples:

**treecolour forest.txt LAI_image.hdr 10.3,-12.4,0.2** 
Apply the mean colour within the tree radius (specified in subtree_radius field if trunks only) from the image startinig at world coord 10.3,-12.4 and with pixel width 0.2m. The colour is added as the red,green,blue fields in the file, it can be seen by running treemesh. This function is useful for mapping foliage density, NDVI or any other colour of false-colour onto the trees.

**treecreate forest 1** 
Generate a tree file of an artifical forest using random seed 1

**treedecimate forest.txt 2 segments**
Reduce the tree to only every 2 segments, maintaining the junction points. So approximately half as detailed.

**treediff forest1.txt forest2.txt**
Compare a forest to a previous version of the forest. Outputs statistics including growth rate, and the volume of woody growth and removal between the dates.

**treefoliage forest.txt forest.ply 0.2**
Add the leaf area density (one-sided leaf area per cubic metre) within 0.2m of the branches onto the branch structure as a greyscale value in the red,green,blue fields.

**treemesh forest.txt**
Convert the tree file into a polygon mesh (.ply file), using the red,green,blue fields as mesh colour if available. 

**treerotate treefile.txt 0,0,30**
rotate a tree file in-place, here by 30 degrees around the z (vertical) axis

**treetranslate treefile.txt 0,0,1**
translate the tree file in-place, here by 1m in the vertical axis


## Unit Tests

UTo build with unit tests, the CMake variable `TREE_BUILD_TESTS` must be `ON`. This can be done in the initial project configuration by running the following command from the `build` directory: `cmake  -DTREE_BUILD_TESTS=ON ..`

Unit tests may then be run by entering the build/bin folder and typing ./treetest