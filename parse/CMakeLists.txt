cmake_minimum_required(VERSION 3.16)
project(parse LANGUAGES CXX)

add_library(parse INTERFACE)
target_include_directories(parse INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parse INTERFACE
  prometheus-cpp::core
  prometheus-cpp::pull
)

target_compile_features(parse INTERFACE cxx_std_20)
