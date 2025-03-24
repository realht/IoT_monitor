# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libsystemd_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libsystemd_FRAMEWORKS_FOUND_RELEASE "${libsystemd_FRAMEWORKS_RELEASE}" "${libsystemd_FRAMEWORK_DIRS_RELEASE}")

set(libsystemd_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libsystemd_DEPS_TARGET)
    add_library(libsystemd_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libsystemd_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libsystemd_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libsystemd_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:libcap::libcap;libmount::libmount;libxcrypt::libxcrypt;libselinux::libselinux;LZ4::lz4_static;LibLZMA::LibLZMA;zstd::libzstd_static>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libsystemd_DEPS_TARGET to all of them
conan_package_library_targets("${libsystemd_LIBS_RELEASE}"    # libraries
                              "${libsystemd_LIB_DIRS_RELEASE}" # package_libdir
                              "${libsystemd_BIN_DIRS_RELEASE}" # package_bindir
                              "${libsystemd_LIBRARY_TYPE_RELEASE}"
                              "${libsystemd_IS_HOST_WINDOWS_RELEASE}"
                              libsystemd_DEPS_TARGET
                              libsystemd_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libsystemd"    # package_name
                              "${libsystemd_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libsystemd_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET libsystemd::libsystemd
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${libsystemd_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${libsystemd_LIBRARIES_TARGETS}>
                 )

    if("${libsystemd_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET libsystemd::libsystemd
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     libsystemd_DEPS_TARGET)
    endif()

    set_property(TARGET libsystemd::libsystemd
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${libsystemd_LINKER_FLAGS_RELEASE}>)
    set_property(TARGET libsystemd::libsystemd
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${libsystemd_INCLUDE_DIRS_RELEASE}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET libsystemd::libsystemd
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Release>:${libsystemd_LIB_DIRS_RELEASE}>)
    set_property(TARGET libsystemd::libsystemd
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${libsystemd_COMPILE_DEFINITIONS_RELEASE}>)
    set_property(TARGET libsystemd::libsystemd
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${libsystemd_COMPILE_OPTIONS_RELEASE}>)

########## For the modules (FindXXX)
set(libsystemd_LIBRARIES_RELEASE libsystemd::libsystemd)
