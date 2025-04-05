# Исполняемый файл
add_executable(analytics_service
  analytics_service.cpp
)

# Зависимости
target_include_directories(analytics_service PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../proto
  ${CMAKE_BINARY_DIR}/proto/generated
)

# Линковка
target_link_libraries(analytics_service PRIVATE
  proto
  protobuf::libprotobuf
  gRPC::grpc++
  gRPC::grpc
  prometheus-cpp::core
  prometheus-cpp::pull
  redis++::redis++_static
  hiredis::hiredis
  Threads::Threads
  parse
)

# Явная зависимость от генерации proto
add_dependencies(analytics_service generate_proto_files)

# Настройки компиляции
set_target_properties(analytics_service PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)
