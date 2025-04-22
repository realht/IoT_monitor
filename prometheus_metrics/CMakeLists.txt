cmake_minimum_required(VERSION 3.16)
project(prometheus_metrics LANGUAGES CXX)

find_package(prometheus-cpp REQUIRED)

add_library(prometheus_metrics STATIC
  prometheus_metrics_publisher.cpp
)
target_include_directories(prometheus_metrics PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(prometheus_metrics PUBLIC
  prometheus-cpp::core
  prometheus-cpp::pull
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
