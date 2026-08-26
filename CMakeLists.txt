cmake_minimum_required(VERSION 3.20)
project(HyperLogLog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(hyperloglog INTERFACE)
target_include_directories(hyperloglog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(murmur3 STATIC
    vendor/murmur3/MurmurHash3.cpp
)
target_include_directories(murmur3 PUBLIC vendor/murmur3)

enable_testing()
add_subdirectory(tests)