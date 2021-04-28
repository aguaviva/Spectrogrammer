cmake_minimum_required(VERSION 3.4.1)
PROJECT(plasma)

include(ExternalProject)

MESSAGE(STATUS "Trying to install fftw...")
ExternalProject_Add(project_fftw
        URL http://www.fftw.org/fftw-3.3.9.tar.gz
        URL_HASH MD5=50145bb68a8510b5d77605f11cadf8dc
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fftw
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     ""
        INSTALL_COMMAND   ""
        TEST_COMMAND      ""
        #CMAKE_ARGS   -DBUILD_TESTS=OFF -DDISABLE_FORTRAN=ON
        )

#add_subdirectory(fftw)
add_library(plasma STATIC IMPORTED)
add_dependencies(plasma project_fftw)
target_link_libraries(plasma fftw )
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-unused-function")

#add_subdirectory(audio)

#add_library(plasma SHARED plasma.cpp colormaps.cpp)
#add_library(plasma SHARED colormaps.cpp)
#add_subdirectory(fftw)
#add_dependencies(plasma project_fftw)
#add_dependencies(plasma audio)
#target_include_directories(plasma PRIVATE fftw/api)
#target_include_directories(plasma PRIVATE audio)

#include_directories(plasma PRIVATE fftw/api)
#include_directories(plasma PRIVATE fftw/api)

#add_subdirectory(fftw)

# Include libraries needed for plasma lib
#target_link_libraries(plasma fftw  android jnigraphics log m OpenSLES atomic )

