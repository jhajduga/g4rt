#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(App)

#----------------------------------------------------------------------------
# Konfiguracja ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED)

#----------------------------------------------------------------------------
# Konfiguracja Geant4
find_package(Geant4 REQUIRED)
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Konfiguracja Pybind11 i Python
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)
find_package(HighFive REQUIRED)


# Ustawienia dla Python.h i bibliotek Pythona
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})
add_definitions(-DPYTHON_EXECUTABLE=\"${Python3_EXECUTABLE}\")

find_package(fmt REQUIRED)
#----------------------------------------------------------------------------
# Konfiguracja Loguru
set(LOGURU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/loguru)
set(LOGURU_SRC ${LOGURU_INCLUDE_DIR}/loguru.cpp)

#----------------------------------------------------------------------------
# Definicje kompilatora
add_definitions(${Core_DEFINITIONS}
                ${Cmd_DEFINITIONS}
                ${IAEA_DEFINITIONS}
                ${TOML_DEFINITIONS}
                ${ConfigSvc_DEFINITIONS}
                ${Geant4_DEFINITIONS}
                ${ROOT_DEFINITIONS})

#----------------------------------------------------------------------------
# Lokalizacja źródeł i nagłówków projektu
set(HEADERS_DIR ${PROJECT_SOURCE_DIR}/basic
                ${PROJECT_SOURCE_DIR}/bworld/src)

include_directories(${HEADERS_DIR}
                    ${IAEA_INCLUDE_DIR}
                    ${Cmd_INCLUDE_DIR}
                    ${Core_INCLUDE_DIRS}
                    ${ConfigSvc_INCLUDE_DIR}
                    ${TOML_INCLUDE_DIR}
                    ${Geant4_INCLUDE_DIRS}
                    ${ROOT_INCLUDE_DIRS}
                    ${LOGURU_INCLUDE_DIR})

# Dodanie biblioteki Core
# add_library(loguru STATIC ${LOGURU_SRC})
# target_compile_definitions(loguru PRIVATE LOGURU_USE_FMTLIB=1)
# Główna aplikacja g4rt
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/basic/*.cc)
add_executable(g4rt ${SOURCES} basic/main.cc ${CMAKE_SOURCE_DIR}/core/services/LogSvc.cpp)
target_link_libraries(g4rt PRIVATE IAEA ConfigSvc Core HighFive ${Geant4_LIBRARIES} dl Python3::Python fmt::fmt)
target_include_directories(g4rt PRIVATE ${ROOT_INCLUDE_DIRS} ${LOGURU_INCLUDE_DIR} ${Python3_INCLUDE_DIRS})
# target_compile_definitions(g4rt PRIVATE LOGURU_USE_FMTLIB=1)

# ----------------------------------------------------------------------------
# Aplikacja g4rt_bworld
file(GLOB_RECURSE SOURCES_BWORLD ${PROJECT_SOURCE_DIR}/bworld/*.cc)
add_executable(g4rt_bworld ${SOURCES_BWORLD} bworld/main.cc ${CMAKE_SOURCE_DIR}/core/services/LogSvc.cpp)
target_link_libraries(g4rt_bworld PRIVATE IAEA ConfigSvc Core HighFive ${Geant4_LIBRARIES} dl Python3::Python fmt::fmt)
target_include_directories(g4rt_bworld PRIVATE ${ROOT_INCLUDE_DIRS} ${LOGURU_INCLUDE_DIR} ${Python3_INCLUDE_DIRS})

# ----------------------------------------------------------------------------
# Aplikacja g4rt_dcm2dat
file(GLOB_RECURSE SOURCES_DCM2DAT ${PROJECT_SOURCE_DIR}/rtplan_converter/*.cc)
add_executable(g4rt_dcm2dat ${SOURCES_DCM2DAT} rtplan_converter/main.cc ${CMAKE_SOURCE_DIR}/core/services/LogSvc.cpp)
target_link_libraries(g4rt_dcm2dat PRIVATE IAEA ConfigSvc Core HighFive ${Geant4_LIBRARIES} dl Python3::Python fmt::fmt)
target_include_directories(g4rt_dcm2dat PRIVATE ${ROOT_INCLUDE_DIRS} ${LOGURU_INCLUDE_DIR} ${Python3_INCLUDE_DIRS})
