cmake_minimum_required(VERSION 3.18)
project(try)
find_package(Boost REQUIRED system thread )
SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)

add_executable(server servermain.cpp header.hpp)
add_executable(client clientmain.cpp header.hpp)

target_compile_features(server  PUBLIC cxx_std_17)
target_compile_options(server  PUBLIC -O2 -Wall -pedantic-errors) 
target_include_directories(server  PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(server ${Boost_LIBRARIES})


target_compile_features(client  PUBLIC cxx_std_17)
target_compile_options(client  PUBLIC -O2 -Wall -Wpedantic) 
target_include_directories(client  PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(client PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(client ${Boost_LIBRARIES})