#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(Core)

#----------------------------------------------------------------------------
# Tell CMake where to find the ROOT installation.
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT COMPONENTS Gdml)

#----------------------------------------------------------------------------
#---Locate the Geant4 package
find_package(Geant4 REQUIRED)

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
include(${Geant4_USE_FILE})

# 

#----------------------------------------------------------------------------
#----------------------------------------------------------------------------
# Setup the module for embedding the python interpreter

find_package(fmt REQUIRED)

#----------------------------------------------------------------------------
# 
set(LOGURU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/loguru)
set(LOGURU_SRC ${LOGURU_INCLUDE_DIR}/loguru.cpp)

#----------------------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})
add_definitions(-DPYTHON_EXECUTABLE=\"${Python3_EXECUTABLE}\")
set(PYBIND11_PYTHON_VERSION ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR})
find_package(pybind11 REQUIRED)
#----------------------------------------------------------------------------
add_definitions(${Geant4_DEFINITIONS}
        ${ConfigSvc_DEFINITIONS}
        ${ROOT_DEFINITIONS}
        ${IAEA_DEFINITIONS})

#----------------------------------------------------------------------------
# Locate sources and headers for this project
#
set(HEADERS_DIR ${PROJECT_SOURCE_DIR}/analysis
        ${PROJECT_SOURCE_DIR}/action
        ${PROJECT_SOURCE_DIR}/physics
        ${PROJECT_SOURCE_DIR}/physics/PrimaryGeneration
        ${PROJECT_SOURCE_DIR}/utilities/
        ${PROJECT_SOURCE_DIR}/utilities/IGeant4
        ${PROJECT_SOURCE_DIR}/geometry/Patient
        ${PROJECT_SOURCE_DIR}/geometry/Patient/WaterPhantom
        ${PROJECT_SOURCE_DIR}/geometry/Patient/SciLab
        ${PROJECT_SOURCE_DIR}/geometry/Patient/DishCube
        ${PROJECT_SOURCE_DIR}/geometry/Patient/D3DDetector
        ${PROJECT_SOURCE_DIR}/geometry/Patient/IbaImRT
        ${PROJECT_SOURCE_DIR}/geometry/Patient/ModularWaterPhantom
        ${PROJECT_SOURCE_DIR}/geometry/Patient/DBGeoBuilder
        ${PROJECT_SOURCE_DIR}/geometry/Linac
        ${PROJECT_SOURCE_DIR}/geometry/Linac/BeamCollimation
        ${PROJECT_SOURCE_DIR}/geometry/Linac/BeamCollimation/MLC
        ${PROJECT_SOURCE_DIR}/geometry/PhaseSpace
        ${PROJECT_SOURCE_DIR}/geometry/BeamMonitoring
        ${PROJECT_SOURCE_DIR}/geometry/WorldConstruction
        ${PROJECT_SOURCE_DIR}/services)

include_directories(${HEADERS_DIR}
        ${IAEA_INCLUDE_DIR}
        ${Geant4_INCLUDE_DIRS}
        ${ConfigSvc_INCLUDE_DIR}
        ${ROOT_INCLUDE_DIRS}
        ${LOGURU_INCLUDE_DIR})

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/action/*.cc
        ${PROJECT_SOURCE_DIR}/analysis/*.cc
        ${PROJECT_SOURCE_DIR}/physics/*.cc
        ${PROJECT_SOURCE_DIR}/geometry/*.cc
        ${PROJECT_SOURCE_DIR}/services/*.cc
        ${PROJECT_SOURCE_DIR}/utilities/*.cc)

#----------------------------------------------------------------------------
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${LOGURU_SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE IAEA ConfigSvc ${Geant4_LIBRARIES} ${ROOT_LIBRARIES} ROOT::Gdml pybind11::embed  fmt::fmt)


#----------------------------------------------------------------------------
# Expose project's public includes to other subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS ${HEADERS_DIR} CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)


# #----------------------------------------------------------------------------
# set(TESTNAME LogServiceTest)
# add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/services/log/test/LogSvcTest.cc)
# target_link_libraries(${TESTNAME} IAEA ConfigSvc Core ${Geant4_LIBRARIES} gtest gtest_main)
# gtest_discover_tests(${TESTNAME})

# ### Test the Dose3D Detector construction
# set(TESTNAME D3DDetectorTest)
# add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/geometry/Patient/D3DDetector/test/D3DDetectorTest.cc)
# target_link_libraries(${TESTNAME} IAEA ConfigSvc Core ${Geant4_LIBRARIES} gtest gtest_main)
# gtest_discover_tests(${TESTNAME})

# ### Test the Water Phantom construction
# set(TESTNAME WaterPhantomTest)
# add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/geometry/Patient/WaterPhantom/test/WaterPhantomTest.cc)
# target_link_libraries(${TESTNAME} IAEA ConfigSvc Core ${Geant4_LIBRARIES} gtest gtest_main)
# gtest_discover_tests(${TESTNAME})
