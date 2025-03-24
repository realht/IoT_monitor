message(STATUS "Conan: Using CMakeDeps conandeps_legacy.cmake aggregator via include()")
message(STATUS "Conan: It is recommended to use explicit find_package() per dependency instead")

find_package(gRPC)
find_package(protobuf)
find_package(redis++)
find_package(hiredis)

set(CONANDEPS_LEGACY  grpc::grpc  protobuf::protobuf  redis++::redis++_static  hiredis::hiredis )