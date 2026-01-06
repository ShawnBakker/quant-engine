cmake_minimum_required(VERSION 3.24)
project(qe_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

enable_testing()
include(CTest)

# Catch2 (tests)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.3
)
FetchContent_MakeAvailable(Catch2)

# Boost (JSON)
set(BOOST_INCLUDE_LIBRARIES json container)
set(BOOST_ENABLE_CMAKE ON)

FetchContent_Declare(
  Boost
  URL https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0-cmake.tar.gz
)
FetchContent_MakeAvailable(Boost)

# qe_engine library
add_library(qe_engine
  src/version.cpp
  src/csv_reader.cpp
  src/indicators.cpp
  src/backtest.cpp
  src/report.cpp
  src/equity_io.cpp
  src/config.cpp
  src/bench.cpp
)

target_include_directories(qe_engine
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(qe_engine
  PUBLIC
    Boost::json
)

# CLI
add_executable(qe_cli
  src/main.cpp
)

target_link_libraries(qe_cli
  PRIVATE
    qe_engine
)

# Tests
add_executable(qe_tests
  tests/test_backtest.cpp
  tests/test_indicators.cpp
  tests/test_report.cpp
  tests/test_config.cpp
)

target_link_libraries(qe_tests
  PRIVATE
    qe_engine
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(qe_tests)
