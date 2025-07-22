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
# Konfiguracja Loguru
set(LOGURU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/loguru)
set(LOGURU_SRC ${LOGURU_INCLUDE_DIR}/loguru.cpp)

#----------------------------------------------------------------------------
find_package(fmt REQUIRED)
#----------------------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})
add_definitions(-DPYTHON_EXECUTABLE=\"${Python3_EXECUTABLE}\")
set(PYBIND11_PYTHON_VERSION ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR})
set(PYBIND11_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/pybind11/include)
find_package(pybind11 REQUIRED)



#----------------------------------------------------------------------------
add_definitions(${Core_DEFINITIONS}
        ${Cmd_DEFINITIONS}
        ${IAEA_DEFINITIONS}
        ${TOML_DEFINITIONS}
        ${ConfigSvc_DEFINITIONS}
        ${Geant4_DEFINITIONS}
        ${ROOT_DEFINITIONS})

set(HEADERS_DIR ${PROJECT_SOURCE_DIR}/basic
                ${PROJECT_SOURCE_DIR}/bworld/src
                ${PROJECT_SOURCE_DIR}/tld/src)

include_directories(${IAEA_INCLUDE_DIR}
        ${Cmd_INCLUDE_DIR}
        ${Core_INCLUDE_DIRS}
        ${ConfigSvc_INCLUDE_DIR}
        ${TOML_INCLUDE_DIR}
        ${Geant4_INCLUDE_DIRS}
        ${ROOT_INCLUDE_DIRS}
        ${LOGURU_INCLUDE_DIR}
        ${HEADERS_DIR})

set(LOG_SVC_SRC ${CMAKE_SOURCE_DIR}/core/services/LogSvc.cpp)
        
#---------------------------------------------------------------------------
if(BUILD_G4RT)
        message(STATUS "BUILD_G4RT is ON: \tBuilding g4rt executable.")
        file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/basic/*.cc)
        add_executable(g4rt ${SOURCES} basic/main.cc ${LOG_SVC_SRC})
        target_link_libraries(g4rt IAEA ConfigSvc Core ${Geant4_LIBRARIES} dl fmt::fmt)
        target_include_directories(g4rt PRIVATE ${LOGURU_INCLUDE_DIR})
        target_compile_options(g4rt PRIVATE -g)
        set_target_properties(g4rt PROPERTIES LINK_FLAGS "-rdynamic")
else()
        message(STATUS "BUILD_G4RT is OFF: \tSkipping build of g4rt executable.")
endif()

#---------------------------------------------------------------------------
if(BUILD_BG4RT)
        message(STATUS "BUILD_BG4RT is ON: \tBuilding g4rt_bworld executable.")
        file(GLOB_RECURSE SOURCES_BWORLD ${PROJECT_SOURCE_DIR}/bworld/*.cc)
        add_executable(g4rt_bworld ${SOURCES_BWORLD} bworld/main.cc ${LOG_SVC_SRC})
        target_link_libraries(g4rt_bworld IAEA ConfigSvc Core ${Geant4_LIBRARIES} dl fmt::fmt)
        target_include_directories(g4rt_bworld PRIVATE ${LOGURU_INCLUDE_DIR})
else()
        message(STATUS "BUILD_BG4RT is OFF: \tSkipping build of g4rt_bworld executable.")
endif()

#---------------------------------------------------------------------------
if(BUILD_DCM2DAT)
        message(STATUS "BUILD_DCM2DAT is ON: \tBuilding dcm2dat executable.")
        file(GLOB_RECURSE SOURCES_DCM2DAT ${PROJECT_SOURCE_DIR}/rtplan_converter/*.cc)
        add_executable(dcm2dat ${SOURCES_DCM2DAT} rtplan_converter/main.cc ${LOG_SVC_SRC})
        target_link_libraries(dcm2dat IAEA ConfigSvc Core ${Geant4_LIBRARIES} dl fmt::fmt)
        target_include_directories(dcm2dat PRIVATE ${LOGURU_INCLUDE_DIR})
else()
        message(STATUS "BUILD_DCM2DAT is OFF: \tSkipping build of dcm2dat executable.")
endif()

#---------------------------------------------------------------------------
if(BUILD_TLD)
        message(STATUS "BUILD_TLD is ON: \tBuilding tld executable.")
        file(GLOB_RECURSE SOURCES_DCM2DAT ${PROJECT_SOURCE_DIR}/tld/*.cc)
        add_executable(tld ${SOURCES_DCM2DAT} tld/main.cc ${LOG_SVC_SRC})
        target_link_libraries(tld IAEA ConfigSvc Core ${Geant4_LIBRARIES} dl fmt::fmt)
        target_include_directories(tld PRIVATE ${LOGURU_INCLUDE_DIR})
else()
        message(STATUS "BUILD_TLD is OFF: \tSkipping build of tld executable.")
endif()

#---------------------------------------------------------------------------
# install definitions
install(TARGETS g4rt
        RUNTIME DESTINATION "${PROJECT_INSTALL_DIR}"
        DESTINATION "${PROJECT_INSTALL_DIR}")
install(DIRECTORY "${PROJECT_DATA_PATH}" DESTINATION "${PROJECT_INSTALL_DIR}/")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/..scripts/g4rt.sh" DESTINATION "/etc/profile.d/")


INCLUDE(CPack)
