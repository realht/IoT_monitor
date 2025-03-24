# === Proto Configuration ===
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/edge_service.proto")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(MAKE_DIRECTORY ${GEN_DIR})

# === Code Generation ===
get_target_property(PROTOC_PATH protobuf::protoc LOCATION)
get_target_property(GRPC_PLUGIN_PATH gRPC::grpc_cpp_plugin LOCATION)

add_custom_command(
  OUTPUT
    "${GEN_DIR}/edge_service.pb.cc"
    "${GEN_DIR}/edge_service.pb.h"
    "${GEN_DIR}/edge_service.grpc.pb.cc"
    "${GEN_DIR}/edge_service.grpc.pb.h"
  COMMAND ${PROTOC_PATH}
  ARGS --grpc_out=generate_mock_code=true:${GEN_DIR}
       --cpp_out=${GEN_DIR}
       --plugin=protoc-gen-grpc=${GRPC_PLUGIN_PATH}
       -I "${CMAKE_CURRENT_SOURCE_DIR}"
       ${PROTO_FILE}
  DEPENDS ${PROTO_FILE}
  COMMENT "Generating Protobuf/gRPC code"
)

# === Proto Library ===
add_library(proto STATIC
  "${GEN_DIR}/edge_service.pb.cc"
  "${GEN_DIR}/edge_service.grpc.pb.cc"
)

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

# Экспорт целей для других поддиректориев
set_target_properties(proto PROPERTIES EXPORT_NAME Proto)
