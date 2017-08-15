# - Try to find SDL2
# Once done, this will define
#
#  SDL2IMAGE_FOUND - system has SDL2
#  SDL2IMAGE_INCLUDE_DIRS - the SDL2 include directories
#  SDL2IMAGE_LIBRARIES - link these to use SDL2

FIND_PACKAGE(PkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
