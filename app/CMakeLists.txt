#CMake版本信息
cmake_minimum_required(VERSION 3.4.1)

#支持-std=gnu++11
set(CMAKE_VERBOSE_MAKEFILE on)

message("--I print it:----------->>>>>>>'${CMAKE_CXX_FLAGS}'")

project(libFFmpeg)
aux_source_directory(src/main/cpp/ DIR_LIB_SRCS)

### 配置资源目录
add_library(native-lib
        SHARED
        ${DIR_LIB_SRCS})

find_library(log-lib
        log)

find_library(android-lib
        android)

# 设置ffmpeg库所在路径的目录
set(distribution_DIR ../../../../libs/armeabi-v7a)

add_library(avutil
        SHARED
        IMPORTED)
set_target_properties(avutil
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavutil-55.so)

add_library(swresample
        SHARED
        IMPORTED)
set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libswresample-2.so)

add_library(avfilter
        SHARED
        IMPORTED)
set_target_properties(avfilter
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavfilter-6.so)

add_library(avformat
        SHARED
        IMPORTED)
set_target_properties(avformat
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavformat-57.so)

add_library(swscale
        SHARED
        IMPORTED)
set_target_properties(swscale
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libswscale-4.so)


add_library(avcodec
        SHARED
        IMPORTED)
set_target_properties(avcodec
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavcodec-57.so)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
# 添加ffmpeg头文件路径
include_directories(libs/include)

target_link_libraries(native-lib
        OpenSLES
        avcodec
        avfilter
        avformat
        avutil
        swresample
        swscale
        ${log-lib}
        ${android-lib})
