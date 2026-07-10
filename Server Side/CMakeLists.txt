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
add_executable (CMakeProject1 "CMakeProject1.cpp" "CMakeProject1.h")



target_link_libraries(CMakeProject1 PRIVATE enet_lib )
if(WIN32)
    target_link_libraries(CMakeProject1 PRIVATE ws2_32 winmm)
    target_compile_options(CMakeProject1 PRIVATE /wd4996) 
elseif(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(CMakeProject1 PRIVATE Threads::Threads)
endif()

