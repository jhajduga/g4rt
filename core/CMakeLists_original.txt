#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
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

#----------------------------------------------------------------------------
# Setup the module for embedding the python interpreter
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 REQUIRED)

#
find_package(spdlog REQUIRED)

#----------------------------------------------------------------------------
add_definitions(${Geant4_DEFINITIONS}
        ${ConfigSvc_DEFINITIONS}
        ${ROOT_DEFINITIONS}
        ${spdlog_DEFINITIONS}
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
        ${PROJECT_SOURCE_DIR}/geometry/Patient/Dose3D
        ${PROJECT_SOURCE_DIR}/geometry/Linac
        ${PROJECT_SOURCE_DIR}/geometry/Linac/BeamCollimation
        ${PROJECT_SOURCE_DIR}/geometry/Linac/BeamCollimation/MLC
        ${PROJECT_SOURCE_DIR}/geometry/PhaseSpace
        ${PROJECT_SOURCE_DIR}/geometry/BeamMonitoring
        ${PROJECT_SOURCE_DIR}/geometry/WorldConstruction
        ${PROJECT_SOURCE_DIR}/services/log
        ${PROJECT_SOURCE_DIR}/services)

include_directories(${HEADERS_DIR}
        ${IAEA_INCLUDE_DIR}
        ${Geant4_INCLUDE_DIRS}
        ${ConfigSvc_INCLUDE_DIR}
        ${spdlog_INCLUDE_DIR}
        ${ROOT_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/action/*.cc
        ${PROJECT_SOURCE_DIR}/analysis/*.cc
        ${PROJECT_SOURCE_DIR}/physics/*.cc
        ${PROJECT_SOURCE_DIR}/geometry/*.cc
        ${PROJECT_SOURCE_DIR}/services/*.cc
        ${PROJECT_SOURCE_DIR}/utilities/*.cc)

#----------------------------------------------------------------------------
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_link_libraries(${PROJECT_NAME} IAEA ConfigSvc ${Geant4_LIBRARIES} ${ROOT_LIBRARIES} ROOT::Gdml spdlog::spdlog pybind11::embed)


#----------------------------------------------------------------------------
# Expose project's public includes to other subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS ${HEADERS_DIR} CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)


#----------------------------------------------------------------------------
set(TESTNAME LogServiceTest)
add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/services/log/test/LogSvcTest.cc)
target_link_libraries(${TESTNAME} IAEA ConfigSvc Core ${Geant4_LIBRARIES} gtest gtest_main)
gtest_discover_tests(${TESTNAME})

### Test the Dose3D Detector construction
set(TESTNAME D3DDetectorTest)
add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/geometry/Patient/Dose3D/test/D3DDetectorTest.cc)
target_link_libraries(${TESTNAME} IAEA ConfigSvc Core spdlog::spdlog ${Geant4_LIBRARIES} gtest gtest_main)
gtest_discover_tests(${TESTNAME})

### Test the Water Phantom construction
set(TESTNAME WaterPhantomTest)
add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/geometry/Patient/WaterPhantom/test/WaterPhantomTest.cc)
target_link_libraries(${TESTNAME} IAEA ConfigSvc Core spdlog::spdlog ${Geant4_LIBRARIES} gtest gtest_main)
gtest_discover_tests(${TESTNAME})
