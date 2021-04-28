cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-unused-function")

add_library(audio SHARED audio_main.cpp audio_recorder.cpp debug_utils.cpp audio_common.cpp )
target_link_libraries(audio
        android
        jnigraphics
        log
        m
        OpenSLES
        atomic)