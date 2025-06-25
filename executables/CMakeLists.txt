#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(Executables)

#----------------------------------------------------------------------------
# Tell CMake where to find the ROOT installation.
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT)

#----------------------------------------------------------------------------
#---Locate the Geant4 package
find_package(Geant4 REQUIRED)

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Setup the module for embedding the python interpreter
find_package(pybind11 REQUIRED)

#
find_package(spdlog REQUIRED)

#----------------------------------------------------------------------------
add_definitions(${Core_DEFINITIONS}
        ${Cmd_DEFINITIONS}
        ${IAEA_DEFINITIONS}
        ${TOML_DEFINITIONS}
        ${ConfigSvc_DEFINITIONS}
        ${Geant4_DEFINITIONS}
        ${spdlog_DEFINITIONS}
        ${ROOT_DEFINITIONS})

include_directories(${IAEA_INCLUDE_DIR}
        ${Cmd_INCLUDE_DIR}
        ${Core_INCLUDE_DIRS}
        ${ConfigSvc_INCLUDE_DIR}
        ${TOML_INCLUDE_DIR}
        ${Geant4_INCLUDE_DIRS}
        ${spdlog_INCLUDE_DIR}
        ${ROOT_INCLUDE_DIRS})

#---------------------------------------------------------------------------
add_executable(g4rt main.cc)
target_link_libraries(g4rt IAEA ConfigSvc Core spdlog::spdlog ${Geant4_LIBRARIES})

#---------------------------------------------------------------------------
# install definitions
install(TARGETS g4rt
        RUNTIME DESTINATION "${PROJECT_INSTALL_DIR}"
        DESTINATION "${PROJECT_INSTALL_DIR}")
install(DIRECTORY "${PROJECT_DATA_PATH}" DESTINATION "${PROJECT_INSTALL_DIR}/")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/g4rt.sh" DESTINATION "/etc/profile.d/")

# deb package definitions
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "madej") #required
SET(CPACK_PACKAGE_VERSION "0.0.5")
SET(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
#SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst")
INCLUDE(CPack)
