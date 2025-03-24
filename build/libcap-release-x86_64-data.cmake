########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND libcap_COMPONENT_NAMES libcap::cap)
list(REMOVE_DUPLICATES libcap_COMPONENT_NAMES)
if(DEFINED libcap_FIND_DEPENDENCY_NAMES)
  list(APPEND libcap_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES libcap_FIND_DEPENDENCY_NAMES)
else()
  set(libcap_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(libcap_PACKAGE_FOLDER_RELEASE "/home/jorik/.conan2/p/b/libcad15372774e7b5/p")
set(libcap_BUILD_MODULES_PATHS_RELEASE )


set(libcap_INCLUDE_DIRS_RELEASE )
set(libcap_RES_DIRS_RELEASE )
set(libcap_DEFINITIONS_RELEASE )
set(libcap_SHARED_LINK_FLAGS_RELEASE )
set(libcap_EXE_LINK_FLAGS_RELEASE )
set(libcap_OBJECTS_RELEASE )
set(libcap_COMPILE_DEFINITIONS_RELEASE )
set(libcap_COMPILE_OPTIONS_C_RELEASE )
set(libcap_COMPILE_OPTIONS_CXX_RELEASE )
set(libcap_LIB_DIRS_RELEASE "${libcap_PACKAGE_FOLDER_RELEASE}/lib")
set(libcap_BIN_DIRS_RELEASE )
set(libcap_LIBRARY_TYPE_RELEASE STATIC)
set(libcap_IS_HOST_WINDOWS_RELEASE 0)
set(libcap_LIBS_RELEASE cap)
set(libcap_SYSTEM_LIBS_RELEASE )
set(libcap_FRAMEWORK_DIRS_RELEASE )
set(libcap_FRAMEWORKS_RELEASE )
set(libcap_BUILD_DIRS_RELEASE )
set(libcap_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(libcap_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libcap_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libcap_COMPILE_OPTIONS_C_RELEASE}>")
set(libcap_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libcap_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libcap_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libcap_EXE_LINK_FLAGS_RELEASE}>")


set(libcap_COMPONENTS_RELEASE libcap::cap)
########### COMPONENT libcap::cap VARIABLES ############################################

set(libcap_libcap_cap_INCLUDE_DIRS_RELEASE )
set(libcap_libcap_cap_LIB_DIRS_RELEASE "${libcap_PACKAGE_FOLDER_RELEASE}/lib")
set(libcap_libcap_cap_BIN_DIRS_RELEASE )
set(libcap_libcap_cap_LIBRARY_TYPE_RELEASE STATIC)
set(libcap_libcap_cap_IS_HOST_WINDOWS_RELEASE 0)
set(libcap_libcap_cap_RES_DIRS_RELEASE )
set(libcap_libcap_cap_DEFINITIONS_RELEASE )
set(libcap_libcap_cap_OBJECTS_RELEASE )
set(libcap_libcap_cap_COMPILE_DEFINITIONS_RELEASE )
set(libcap_libcap_cap_COMPILE_OPTIONS_C_RELEASE "")
set(libcap_libcap_cap_COMPILE_OPTIONS_CXX_RELEASE "")
set(libcap_libcap_cap_LIBS_RELEASE cap)
set(libcap_libcap_cap_SYSTEM_LIBS_RELEASE )
set(libcap_libcap_cap_FRAMEWORK_DIRS_RELEASE )
set(libcap_libcap_cap_FRAMEWORKS_RELEASE )
set(libcap_libcap_cap_DEPENDENCIES_RELEASE )
set(libcap_libcap_cap_SHARED_LINK_FLAGS_RELEASE )
set(libcap_libcap_cap_EXE_LINK_FLAGS_RELEASE )
set(libcap_libcap_cap_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(libcap_libcap_cap_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libcap_libcap_cap_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libcap_libcap_cap_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libcap_libcap_cap_EXE_LINK_FLAGS_RELEASE}>
)
set(libcap_libcap_cap_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libcap_libcap_cap_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libcap_libcap_cap_COMPILE_OPTIONS_C_RELEASE}>")