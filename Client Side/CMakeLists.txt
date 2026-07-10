# CMakeList.txt : CMake project for Ceva game client, include source and define
# project specific logic here.
#

cmake_minimum_required(VERSION 3.16)
project(CevaGameClient CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(WIN32 _WINDOWS HAS_WINSOCK2 _AMD64_)
endif()
file(GLOB ENET_SOURCES "enetOG/*.c")

add_library(enet_lib STATIC ${ENET_SOURCES})
target_include_directories(enet_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/enetOG/include")
add_executable (CMakeTarget "Ceva game client.cpp" "Ceva game client.h" )
target_link_libraries(CMakeTarget PRIVATE enet_lib)
if(WIN32)
    target_link_libraries(CMakeTarget PRIVATE ws2_32 winmm)
    target_compile_options(CMakeTarget PRIVATE /wd4996) 
    target_include_directories(CMakeTarget PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/raylib/include")
    target_link_libraries(CMakeTarget PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/raylib/lib/raylib.lib" winmm)
elseif(UNIX AND NOT APPLE)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
    )
    FetchContent_MakeAvailable(raylib)
    find_package(Threads REQUIRED)
    target_link_libraries(CMakeTarget PRIVATE 
        raylib
        GL
        X11
        Xrandr
        Xinerama
        Xi
        Xcursor
        m
        dl
        Threads::Threads
    )
endif()



# TODO: Add tests and install targets if needed.
