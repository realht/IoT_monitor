########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(libxcrypt_COMPONENT_NAMES "")
if(DEFINED libxcrypt_FIND_DEPENDENCY_NAMES)
  list(APPEND libxcrypt_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES libxcrypt_FIND_DEPENDENCY_NAMES)
else()
  set(libxcrypt_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(libxcrypt_PACKAGE_FOLDER_RELEASE "/home/jorik/.conan2/p/b/libxcb09c840400726/p")
set(libxcrypt_BUILD_MODULES_PATHS_RELEASE )


set(libxcrypt_INCLUDE_DIRS_RELEASE )
set(libxcrypt_RES_DIRS_RELEASE )
set(libxcrypt_DEFINITIONS_RELEASE )
set(libxcrypt_SHARED_LINK_FLAGS_RELEASE )
set(libxcrypt_EXE_LINK_FLAGS_RELEASE )
set(libxcrypt_OBJECTS_RELEASE )
set(libxcrypt_COMPILE_DEFINITIONS_RELEASE )
set(libxcrypt_COMPILE_OPTIONS_C_RELEASE )
set(libxcrypt_COMPILE_OPTIONS_CXX_RELEASE )
set(libxcrypt_LIB_DIRS_RELEASE "${libxcrypt_PACKAGE_FOLDER_RELEASE}/lib")
set(libxcrypt_BIN_DIRS_RELEASE )
set(libxcrypt_LIBRARY_TYPE_RELEASE STATIC)
set(libxcrypt_IS_HOST_WINDOWS_RELEASE 0)
set(libxcrypt_LIBS_RELEASE crypt)
set(libxcrypt_SYSTEM_LIBS_RELEASE )
set(libxcrypt_FRAMEWORK_DIRS_RELEASE )
set(libxcrypt_FRAMEWORKS_RELEASE )
set(libxcrypt_BUILD_DIRS_RELEASE )
set(libxcrypt_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(libxcrypt_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libxcrypt_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libxcrypt_COMPILE_OPTIONS_C_RELEASE}>")
set(libxcrypt_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libxcrypt_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libxcrypt_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libxcrypt_EXE_LINK_FLAGS_RELEASE}>")


set(libxcrypt_COMPONENTS_RELEASE )