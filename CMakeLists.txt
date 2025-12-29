cmake_minimum_required(VERSION 3.10)
project(SimpleFileIOTests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Your library headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Tell CMake where to look for Catch2 config (Brew path)
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/lib/cmake/Catch2")

# Find Catch2 (requires the config file)
find_package(Catch2 3 REQUIRED)

# Build test executable
add_executable(tests
    tests/test_file.cpp
)

# Link Catch2
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)