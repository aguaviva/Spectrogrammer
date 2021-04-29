cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(audio)
INCLUDE_DIRECTORIES(audio)

set(BUILD_TESTS OFF)
set(DISABLE_FORTRAN OFF)
add_subdirectory(fftw-3.3.9)
INCLUDE_DIRECTORIES(fftw-3.3.9/api)

add_library(plasma SHARED plasma.cpp colormaps.cpp)

TARGET_LINK_LIBRARIES(plasma audio fftw3)