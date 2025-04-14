# Исполняемый файл
add_executable(edge_service
  edge_server.cpp
  edge_service.cpp
)

# Зависимости
target_include_directories(edge_service PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../config
  ${CMAKE_CURRENT_SOURCE_DIR}/../proto
  ${CMAKE_BINARY_DIR}/proto/generated
)

# Линковка
target_link_libraries(edge_service PRIVATE
  proto
  protobuf::libprotobuf
  gRPC::grpc++
  gRPC::grpc
  Threads::Threads
  redis++::redis++_static
  hiredis::hiredis
  parse
)

# Явная зависимость от генерации proto
add_dependencies(edge_service generate_proto_files)

# Настройки компиляции
set_target_properties(edge_service PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)
