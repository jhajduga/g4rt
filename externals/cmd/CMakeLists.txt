#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(Cmd)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
#
set(HEADERS_DIR ${PROJECT_SOURCE_DIR}/include)
include_directories(${HEADERS_DIR})

#----------------------------------------------------------------------------
# Expose project's public includes to other subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIR ${HEADERS_DIR} CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
