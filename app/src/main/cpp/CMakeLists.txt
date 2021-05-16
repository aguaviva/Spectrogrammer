cmake_minimum_required(VERSION 3.9.4)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -flto")

add_subdirectory(audio)
INCLUDE_DIRECTORIES(audio)

set(BUILD_TESTS OFF)
set(DISABLE_FORTRAN OFF)
add_subdirectory(fftw-3.3.9)
INCLUDE_DIRECTORIES(fftw-3.3.9/api)

add_library(plasma SHARED plasma.cpp colormaps.cpp scale.cpp waterfall.cpp auformat.cpp ChunkerProcessor.cpp)

TARGET_LINK_LIBRARIES(plasma audio fftw3)

