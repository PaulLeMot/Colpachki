cmake_minimum_required(VERSION 3.16)
project(Colpachki)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

add_executable(Colpachki 
    src/Main.cpp
)

target_include_directories(Colpachki PRIVATE include)

target_link_libraries(Colpachki 
    PRIVATE 
    SDL3::SDL3 
    SDL3_ttf::SDL3_ttf
)
