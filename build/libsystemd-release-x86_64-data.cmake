########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(libsystemd_COMPONENT_NAMES "")
if(DEFINED libsystemd_FIND_DEPENDENCY_NAMES)
  list(APPEND libsystemd_FIND_DEPENDENCY_NAMES libcap libmount libxcrypt libselinux lz4 LibLZMA zstd)
  list(REMOVE_DUPLICATES libsystemd_FIND_DEPENDENCY_NAMES)
else()
  set(libsystemd_FIND_DEPENDENCY_NAMES libcap libmount libxcrypt libselinux lz4 LibLZMA zstd)
endif()
set(libcap_FIND_MODE "NO_MODULE")
set(libmount_FIND_MODE "NO_MODULE")
set(libxcrypt_FIND_MODE "NO_MODULE")
set(libselinux_FIND_MODE "NO_MODULE")
set(lz4_FIND_MODE "NO_MODULE")
set(LibLZMA_FIND_MODE "NO_MODULE")
set(zstd_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(libsystemd_PACKAGE_FOLDER_RELEASE "/home/jorik/.conan2/p/b/libsy2afec29fbbea4/p")
set(libsystemd_BUILD_MODULES_PATHS_RELEASE )


set(libsystemd_INCLUDE_DIRS_RELEASE )
set(libsystemd_RES_DIRS_RELEASE )
set(libsystemd_DEFINITIONS_RELEASE )
set(libsystemd_SHARED_LINK_FLAGS_RELEASE )
set(libsystemd_EXE_LINK_FLAGS_RELEASE )
set(libsystemd_OBJECTS_RELEASE )
set(libsystemd_COMPILE_DEFINITIONS_RELEASE )
set(libsystemd_COMPILE_OPTIONS_C_RELEASE )
set(libsystemd_COMPILE_OPTIONS_CXX_RELEASE )
set(libsystemd_LIB_DIRS_RELEASE "${libsystemd_PACKAGE_FOLDER_RELEASE}/lib")
set(libsystemd_BIN_DIRS_RELEASE )
set(libsystemd_LIBRARY_TYPE_RELEASE STATIC)
set(libsystemd_IS_HOST_WINDOWS_RELEASE 0)
set(libsystemd_LIBS_RELEASE systemd)
set(libsystemd_SYSTEM_LIBS_RELEASE rt pthread dl)
set(libsystemd_FRAMEWORK_DIRS_RELEASE )
set(libsystemd_FRAMEWORKS_RELEASE )
set(libsystemd_BUILD_DIRS_RELEASE )
set(libsystemd_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(libsystemd_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libsystemd_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libsystemd_COMPILE_OPTIONS_C_RELEASE}>")
set(libsystemd_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libsystemd_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libsystemd_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libsystemd_EXE_LINK_FLAGS_RELEASE}>")


set(libsystemd_COMPONENTS_RELEASE )