# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release

# Include any dependencies generated for this target.
include 3rd_party/rply/CMakeFiles/rply.dir/depend.make

# Include the progress variables for this target.
include 3rd_party/rply/CMakeFiles/rply.dir/progress.make

# Include the compile flags for this target's objects.
include 3rd_party/rply/CMakeFiles/rply.dir/flags.make

3rd_party/rply/CMakeFiles/rply.dir/rply.c.o: 3rd_party/rply/CMakeFiles/rply.dir/flags.make
3rd_party/rply/CMakeFiles/rply.dir/rply.c.o: ../3rd_party/rply/rply.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object 3rd_party/rply/CMakeFiles/rply.dir/rply.c.o"
	cd /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/rply.dir/rply.c.o   -c /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/3rd_party/rply/rply.c

3rd_party/rply/CMakeFiles/rply.dir/rply.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/rply.dir/rply.c.i"
	cd /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/3rd_party/rply/rply.c > CMakeFiles/rply.dir/rply.c.i

3rd_party/rply/CMakeFiles/rply.dir/rply.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/rply.dir/rply.c.s"
	cd /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/3rd_party/rply/rply.c -o CMakeFiles/rply.dir/rply.c.s

# Object files for target rply
rply_OBJECTS = \
"CMakeFiles/rply.dir/rply.c.o"

# External object files for target rply
rply_EXTERNAL_OBJECTS =

lib/librply.a: 3rd_party/rply/CMakeFiles/rply.dir/rply.c.o
lib/librply.a: 3rd_party/rply/CMakeFiles/rply.dir/build.make
lib/librply.a: 3rd_party/rply/CMakeFiles/rply.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library ../../lib/librply.a"
	cd /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply && $(CMAKE_COMMAND) -P CMakeFiles/rply.dir/cmake_clean_target.cmake
	cd /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/rply.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
3rd_party/rply/CMakeFiles/rply.dir/build: lib/librply.a

.PHONY : 3rd_party/rply/CMakeFiles/rply.dir/build

3rd_party/rply/CMakeFiles/rply.dir/clean:
	cd /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply && $(CMAKE_COMMAND) -P CMakeFiles/rply.dir/cmake_clean.cmake
.PHONY : 3rd_party/rply/CMakeFiles/rply.dir/clean

3rd_party/rply/CMakeFiles/rply.dir/depend:
	cd /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/3rd_party/rply /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply/CMakeFiles/rply.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : 3rd_party/rply/CMakeFiles/rply.dir/depend

