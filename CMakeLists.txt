cmake_minimum_required(VERSION 3.9)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(Tetris VERSION 1.0
    DESCRIPTION "Tetris 2009 Spec-compliant C++ implementation"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-Wall -Wextra -pedantic -Werror)
add_executable(tetris lib/tetris.cpp lib/helper.hpp lib/all_headers.h)

# target_compile_options(tetris)
set_target_properties(tetris PROPERTIES CXX_STANDARD 23 )

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.10 # latest Catch2 V2 version -- can't use V3 as ApproveTests doesn't support it 
)

FetchContent_MakeAvailable(Catch2)

add_executable(test_tetris test/main.cpp test/test_tetris.cpp)
add_executable(test_shapes test/main.cpp test/test_shapes.cpp)
target_link_libraries(test_tetris Catch2::Catch2)
target_link_libraries(test_shapes Catch2::Catch2)
