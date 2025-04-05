cmake_minimum_required(VERSION 3.16)
project(RealTimeIoT LANGUAGES CXX)

# Глобальные настройки
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Настройка путей для Protobuf
set(Protobuf_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto)

# Интеграция Conan
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

# Поиск зависимостей
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Threads REQUIRED)
find_package(redis++ REQUIRED)
find_package(hiredis REQUIRED)
find_package(prometheus-cpp REQUIRED)

# Поддиректории (важен порядок!)
add_subdirectory(proto)
add_subdirectory(parse)
add_subdirectory(edge_service)
add_subdirectory(analytics_service)

# Явные зависимости между целями
add_dependencies(edge_service generate_proto_files)
add_dependencies(analytics_service generate_proto_files)
