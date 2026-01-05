cmake_minimum_required(VERSION 3.20)
project(qe_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# warnings
if (MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# lib
add_library(qe_engine
  src/version.cpp
  src/csv_reader.cpp
  src/indicators.cpp
)

target_include_directories(qe_engine
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# CLI executable
add_executable(qe_cli
  src/main.cpp
)

target_link_libraries(qe_cli PRIVATE qe_engine)
