#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(G4rt)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_DEFAULT 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

#----------------------------------------------------------------------------
# prevent for in-source building
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif ()

#----------------------------------------------------------------------------
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # This needs to be done universally to any static library

#----------------------------------------------------------------------------
if(NOT $ENV{CLHEP_INCLUDE_DIR} STREQUAL "")
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES $ENV{CLHEP_INCLUDE_DIR})
endif ()
#----------------------------------------------------------------------------
SET(PROJECT_DATA_PATH "${PROJECT_SOURCE_DIR}/data")
SET(PROJECT_INSTALL_DIR "/opt/g4rt")
add_definitions(-DPROJECT_LOCATION_PATH=\"${PROJECT_SOURCE_DIR}\")
add_definitions(-DPROJECT_DATA_PATH=\"${PROJECT_DATA_PATH}\")
add_definitions(-DLHCBD1_DATA_PATH=\"/data3/TN-Dose3D/data\")
add_definitions(-DBR_DATA_PATH=\"/mnt/e/Data/dose3d\")
add_definitions(-DJH_DATA_PATH=\"/mnt/d/Data\")
add_definitions(-DJOBS_PATH=\"${PROJECT_SOURCE_DIR}/jobs\")
add_definitions(-DPROJECT_PY_PATH=\"${PROJECT_SOURCE_DIR}/core/utilities/python\")
add_definitions(-DPROJECT_BINARY_PATH=\"${PROJECT_BINARY_DIR}\")

#----------------------------------------------------------------------------
# Testing suite configuration
#
find_package(GTest REQUIRED)
include(GoogleTest)
include(CTest)

#----------------------------------------------------------------------------
# Configure the components (subprojects)
# NOTE: The order matters! The most independent ones should go first.
#       If A and B really depend on each other, CMake  can build such correctly,
#       if you specify full path to the include directlry
add_subdirectory(externals)
add_subdirectory(core)
# add_subdirectory(executables)
add_subdirectory(app)

#----------------------------------------------------------------------------
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})


#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build g4af. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(RUN_SCRIPTS
    scoring_pre.mac
    scoring_post.mac
    drawProfiles.py
    testGeo.C
)

foreach(_script ${RUN_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/scripts/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()