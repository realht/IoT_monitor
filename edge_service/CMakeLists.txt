# === Executable ===
add_executable(edge_service
  edge_service.cpp
)

# Подключаем сгенерированные заголовки
target_include_directories(edge_service PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../proto
  ${CMAKE_CURRENT_BINARY_DIR}/proto/generated
)

# Линковка зависимостей
target_link_libraries(edge_service PRIVATE
  proto
  protobuf::libprotobuf
  gRPC::grpc++
  gRPC::grpc
  Threads::Threads
  redis++::redis++_static
  hiredis::hiredis
)
