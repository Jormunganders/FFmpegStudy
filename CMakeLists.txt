cmake_minimum_required(VERSION 3.15)
project(ffmpeg)

set(CMAKE_CXX_STANDARD 17)

include_directories(include/)
link_directories(libs/)

set(SOURCE_FILES
        src/main.cpp src/avformat_use.h src/sdl_use.h src/audio_use.h)

add_executable(ffmpeg
        ${SOURCE_FILES})

target_link_libraries(
        ffmpeg
        avcodec.58.35.100
        avdevice.58.5.100
        avfilter.7.40.101
        avformat.58.20.100
        avresample.4.0.0
        avutil.56.22.100
        swresample.3.3.100
        swscale.5.3.100
        SDL2-2.0.0
        SDL2_image-2.0.0
)