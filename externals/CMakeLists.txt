#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(Externals)

#----------------------------------------------------------------------------
# Konfiguracja komponentów (podprojektów)
# UWAGA: Kolejność ma znaczenie! Najbardziej niezależne komponenty powinny być dodane jako pierwsze.

# Znajdowanie Loguru
set(LOGURU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/loguru)
set(LOGURU_SRC ${LOGURU_INCLUDE_DIR}/loguru.cpp)

# Podprojekt cmd (interfejs użytkownika tekstowego)
add_subdirectory(cmd)

#----------------------------------------------------------------------------
# Konfiguracja podsystemu IAEA
add_subdirectory(iaea)

#----------------------------------------------------------------------------
# Konfiguracja ConfigSvc
add_subdirectory(config/library)
