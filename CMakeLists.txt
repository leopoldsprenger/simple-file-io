cmake_minimum_required(VERSION 3.10)
project(simple-file-io)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directory for headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add source files
add_executable(${PROJECT_NAME}
    src/file.cpp
)