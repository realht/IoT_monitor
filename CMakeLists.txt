cmake_minimum_required(VERSION 3.16)
project(RealTimeIoT LANGUAGES CXX)

include_directories(
  ${CMAKE_BINARY_DIR}/proto/generated
  /home/jorik/.conan2/p/b/grpc818aebdb9f01a/p/include
)

# === Global Settings ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Conan Integration ===
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

# === Global Dependencies ===
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Threads REQUIRED)
find_package(redis++ REQUIRED)
find_package(hiredis REQUIRED)

# === Subdirectories ===
add_subdirectory(proto)
add_subdirectory(edge_service)
