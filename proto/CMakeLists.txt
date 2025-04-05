# Генерация кода Protobuf
set(COMMON_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/common.proto")
set(EDGE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/edge_service.proto")
set(ANALYTICS_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/analytics_service.proto")
set(GEN_DIR "${CMAKE_BINARY_DIR}/proto/generated")

file(MAKE_DIRECTORY ${GEN_DIR})

# Получение путей к компиляторам
get_target_property(PROTOC_PATH protobuf::protoc LOCATION)
get_target_property(GRPC_PLUGIN_PATH gRPC::grpc_cpp_plugin LOCATION)

# Кастомная команда для генерации
add_custom_command(
  OUTPUT
    "${GEN_DIR}/common.pb.cc"
    "${GEN_DIR}/common.pb.h"
    "${GEN_DIR}/edge_service.pb.cc"
    "${GEN_DIR}/edge_service.pb.h"
    "${GEN_DIR}/edge_service.grpc.pb.cc"
    "${GEN_DIR}/edge_service.grpc.pb.h"
    "${GEN_DIR}/analytics_service.pb.cc"
    "${GEN_DIR}/analytics_service.pb.h"
    "${GEN_DIR}/analytics_service.grpc.pb.cc"
    "${GEN_DIR}/analytics_service.grpc.pb.h"
  COMMAND ${PROTOC_PATH}
  ARGS
    --grpc_out=generate_mock_code=true:${GEN_DIR}
    --cpp_out=${GEN_DIR}
    --plugin=protoc-gen-grpc=${GRPC_PLUGIN_PATH}
    -I "${CMAKE_CURRENT_SOURCE_DIR}"
    ${COMMON_PROTO} ${EDGE_PROTO} ${ANALYTICS_PROTO}
  DEPENDS ${COMMON_PROTO} ${EDGE_PROTO} ${ANALYTICS_PROTO}
  COMMENT "Generating Protobuf/gRPC code"
)

# Отдельная цель для генерации
add_custom_target(generate_proto_files DEPENDS
  "${GEN_DIR}/common.pb.cc"
  "${GEN_DIR}/edge_service.grpc.pb.cc"
  "${GEN_DIR}/analytics_service.grpc.pb.cc"
)

# OBJECT-библиотека для proto
add_library(proto OBJECT
  "${GEN_DIR}/common.pb.cc"
  "${GEN_DIR}/edge_service.pb.cc"
  "${GEN_DIR}/edge_service.grpc.pb.cc"
  "${GEN_DIR}/analytics_service.pb.cc"
  "${GEN_DIR}/analytics_service.grpc.pb.cc"
)

# Зависимости и линковка
target_include_directories(proto PUBLIC
  ${GEN_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(proto PUBLIC
  protobuf::libprotobuf
  gRPC::grpc++
  gRPC::grpc
  Threads::Threads
)

# Экспорт целей
set_target_properties(proto PROPERTIES
  EXPORT_NAME Proto
  POSITION_INDEPENDENT_CODE ON
)
