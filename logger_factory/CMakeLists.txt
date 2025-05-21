cmake_minimum_required(VERSION 3.16)
project(logger LANGUAGES CXX)

find_package(spdlog REQUIRED)

add_library(logger INTERFACE)

target_include_directories(logger INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../config
)

target_link_libraries(logger INTERFACE
  spdlog::spdlog
)

target_compile_features(logger INTERFACE cxx_std_20)