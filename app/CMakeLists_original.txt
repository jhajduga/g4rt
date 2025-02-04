#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
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
set(PYBIND11_FINDPYTHON ON)
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

set(HEADERS_DIR ${PROJECT_SOURCE_DIR}/basic
                ${PROJECT_SOURCE_DIR}/bworld/src)

include_directories(${IAEA_INCLUDE_DIR}
        ${Cmd_INCLUDE_DIR}
        ${Core_INCLUDE_DIRS}
        ${ConfigSvc_INCLUDE_DIR}
        ${TOML_INCLUDE_DIR}
        ${Geant4_INCLUDE_DIRS}
        ${spdlog_INCLUDE_DIR}
        ${ROOT_INCLUDE_DIRS}
        ${HEADERS_DIR})

        
#---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/basic/*.cc)
add_executable(g4rt ${SOURCES} basic/main.cc)
target_link_libraries(g4rt IAEA ConfigSvc Core spdlog::spdlog ${Geant4_LIBRARIES})

#---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES_BWORLD ${PROJECT_SOURCE_DIR}/bworld/*.cc)
add_executable(g4rt_bworld ${SOURCES_BWORLD} bworld/main.cc)
target_link_libraries(g4rt_bworld IAEA ConfigSvc Core spdlog::spdlog ${Geant4_LIBRARIES})

#---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES_DCM2DAT ${PROJECT_SOURCE_DIR}/rtplan_converter/*.cc)
add_executable(g4rt_dcm2dat ${SOURCES_DCM2DAT} rtplan_converter/main.cc)
target_link_libraries(g4rt_dcm2dat IAEA ConfigSvc Core spdlog::spdlog ${Geant4_LIBRARIES})

#---------------------------------------------------------------------------
# install definitions
install(TARGETS g4rt
        RUNTIME DESTINATION "${PROJECT_INSTALL_DIR}"
        DESTINATION "${PROJECT_INSTALL_DIR}")
install(DIRECTORY "${PROJECT_DATA_PATH}" DESTINATION "${PROJECT_INSTALL_DIR}/")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/..scripts/g4rt.sh" DESTINATION "/etc/profile.d/")

# deb package definitions
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "madej") #required
SET(CPACK_PACKAGE_VERSION "0.0.5")
SET(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
#SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst")
INCLUDE(CPack)
