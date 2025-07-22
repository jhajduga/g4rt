#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(Externals)

#----------------------------------------------------------------------------
# Configure the components (subprojects)
# NOTE: The order matters! The most independent ones should go first.
#----------------------------------------------------------------------------

# 
set(LOGURU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/loguru)


# cmd for building text user interface
add_subdirectory(cmd)

#----------------------------------------------------------------------------
# iaea
add_subdirectory(iaea)

#----------------------------------------------------------------------------
# ConfigSvc
add_subdirectory(config/library)
