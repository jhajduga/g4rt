#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(Core)

#----------------------------------------------------------------------------
# Konfiguracja ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#---Locate the ROOT package
find_package(ROOT COMPONENTS Gdml)

#----------------------------------------------------------------------------
# Konfiguracja Geant4
find_package(Geant4 REQUIRED)

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
include(${Geant4_USE_FILE})

find_package(fmt REQUIRED)

#----------------------------------------------------------------------------
# Konfiguracja Pybind11 i Python
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

# Ustawienia dla Python.h i bibliotek Pythona
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})
add_definitions(-DPYTHON_EXECUTABLE=\"${Python3_EXECUTABLE}\")

#----------------------------------------------------------------------------
# Konfiguracja Loguru
set(LOGURU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/loguru)
set(LOGURU_SRC ${LOGURU_INCLUDE_DIR}/loguru.cpp)

#----------------------------------------------------------------------------
# Konfiguracja Loguru
set(LOGURU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/loguru)
set(LOGURU_SRC ${LOGURU_INCLUDE_DIR}/loguru.cpp)

#----------------------------------------------------------------------------
add_definitions(${Geant4_DEFINITIONS}
        ${ConfigSvc_DEFINITIONS}
        ${ROOT_DEFINITIONS}
        ${IAEA_DEFINITIONS})

#----------------------------------------------------------------------------
# Lokalizacja źródeł i nagłówków projektu
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
# Tworzenie biblioteki Core
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${LOGURU_SRC})

#----------------------------------------------------------------------------
# Linkowanie bibliotek
# Loguru działa jako nagłówkowy, więc dodatkowe linkowanie nie jest wymagane
# Jednak plik źródłowy loguru.cpp jest dodawany do biblioteki

target_link_libraries(${PROJECT_NAME} PRIVATE IAEA ConfigSvc ${Geant4_LIBRARIES} ${ROOT_LIBRARIES} ROOT::Gdml pybind11::embed fmt::fmt)

#----------------------------------------------------------------------------
# Expose project's public includes to other subprojects
set(${PROJECT_NAME}_INCLUDE_DIRS ${HEADERS_DIR} CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

### Test the Dose3D Detector construction
set(TESTNAME D3DDetectorTest)
add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/geometry/Patient/Dose3D/test/D3DDetectorTest.cc ${LOGURU_SRC})
target_link_libraries(${TESTNAME} IAEA ConfigSvc Core ${Geant4_LIBRARIES} gtest gtest_main dl Python3::Python fmt::fmt)
target_include_directories(${TESTNAME} PRIVATE ${ROOT_INCLUDE_DIRS} ${LOGURU_INCLUDE_DIR} ${Python3_INCLUDE_DIRS})
gtest_discover_tests(${TESTNAME})

### Test the Water Phantom construction
set(TESTNAME WaterPhantomTest)
add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/geometry/Patient/WaterPhantom/test/WaterPhantomTest.cc ${LOGURU_SRC})
target_link_libraries(${TESTNAME} IAEA ConfigSvc Core ${Geant4_LIBRARIES} gtest gtest_main dl Python3::Python fmt::fmt)
target_include_directories(${TESTNAME} PRIVATE ${ROOT_INCLUDE_DIRS} ${LOGURU_INCLUDE_DIR} ${Python3_INCLUDE_DIRS})
gtest_discover_tests(${TESTNAME})