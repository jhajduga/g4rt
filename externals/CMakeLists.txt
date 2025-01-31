#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(Externals)

#----------------------------------------------------------------------------
# Konfiguracja komponentów (podprojektów)
# UWAGA: Kolejność ma znaczenie! Najbardziej niezależne komponenty powinny być dodane jako pierwsze.

#----------------------------------------------------------------------------
# Dodanie ścieżek dla Loguru (loguru.cpp będzie dołączany tylko w projekcie Core)
set(LOGURU_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/loguru)

#----------------------------------------------------------------------------
# Dodanie podprojektów

# Podprojekt cmd (interfejs użytkownika tekstowego)
add_subdirectory(cmd)

# Podprojekt IAEA
add_subdirectory(iaea)

# Podprojekt ConfigSvc (biblioteka konfiguracji)
add_subdirectory(config/library)

