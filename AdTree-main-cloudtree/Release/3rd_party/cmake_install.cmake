# Install script for directory: /media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/3rd_party

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/glew/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/glfw/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/tetgen/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/kd_tree/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/cminpack/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/optimizer_lm/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/easy3d/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/imgui/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/tinyfiledialogs/cmake_install.cmake")
  include("/media/sun017/98c26b76-de0f-4a92-98d1-a170710911e93/home/sun017/MyFiles/HazardTrees/codes/AdTree/AdTree-main-cloudtree/Release/3rd_party/rply/cmake_install.cmake")

endif()

