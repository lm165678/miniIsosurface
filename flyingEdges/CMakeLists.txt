# miniIsosurface is distributed under the OSI-approved BSD 3-clause License.
# See LICENSE.txt for details.

# Copyright (c) 2017
# National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
# the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
# certain rights in this software.

cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)

project(flyingEdges)

option(BUILD_CUDA OFF)
option(BUILD_OPENMP OFF)

add_subdirectory(serial)

if (BUILD_CUDA)
    find_package(CUDA REQUIRED)
    add_subdirectory(thrust)
    #add_subdirectory(cuda)
    #add_subdirectory(cuda2)
endif()

if (BUILD_OPENMP)
    add_subdirectory(openmp)
endif()
