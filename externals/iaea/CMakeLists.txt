#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(IAEA)

#----------------------------------------------------------------------------
#---Locate the Geant4 package
find_package(Geant4 REQUIRED)

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
add_definitions(${Geant4_DEFINITIONS})

#----------------------------------------------------------------------------
# Locate sources and headers for this project
#
set(HEADERS_DIR ${PROJECT_SOURCE_DIR}/include)
set(SOURCES_DIR ${PROJECT_SOURCE_DIR}/src)

include_directories(${HEADERS_DIR}
        ${Geant4_INCLUDE_DIRS})

set(SOURCES ${SOURCES_DIR}/G4IAEAphspReader.cc
        ${SOURCES_DIR}/iaea_header.cc
        ${SOURCES_DIR}/iaea_phsp.cc
        ${SOURCES_DIR}/iaea_record.cc
        ${SOURCES_DIR}/utilities.cc)

#----------------------------------------------------------------------------
# This needs to be done universally to any static library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define the library
add_library(${PROJECT_NAME} STATIC ${SOURCES})

#----------------------------------------------------------------------------
# Expose project's public includes to other subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIR ${HEADERS_DIR} CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

