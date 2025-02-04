#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(Externals)

#----------------------------------------------------------------------------
# Configure the components (subprojects)
# NOTE: The order matters! The most independent ones should go first.
#----------------------------------------------------------------------------

# cmd for building text user interface
add_subdirectory(cmd)

#----------------------------------------------------------------------------
# iaea
add_subdirectory(iaea)

#----------------------------------------------------------------------------
# ConfigSvc
add_subdirectory(config/library)
