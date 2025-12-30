cmake_minimum_required(VERSION 3.10)
project(SimpleFileIOTests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/lib/cmake/Catch2")

# Find Catch2 (requires the config file)
find_package(Catch2 3 REQUIRED)

add_executable(tests
    tests/test_file.cpp
)

# Link Catch2
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)


add_executable(benchmark
    bench/benchmark_all.cpp
)

# Enable high optimization for benchmarks
target_compile_options(benchmark PRIVATE -O3)