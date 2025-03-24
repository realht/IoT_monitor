# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libxcrypt_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libxcrypt_FRAMEWORKS_FOUND_RELEASE "${libxcrypt_FRAMEWORKS_RELEASE}" "${libxcrypt_FRAMEWORK_DIRS_RELEASE}")

set(libxcrypt_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libxcrypt_DEPS_TARGET)
    add_library(libxcrypt_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libxcrypt_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libxcrypt_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libxcrypt_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libxcrypt_DEPS_TARGET to all of them
conan_package_library_targets("${libxcrypt_LIBS_RELEASE}"    # libraries
                              "${libxcrypt_LIB_DIRS_RELEASE}" # package_libdir
                              "${libxcrypt_BIN_DIRS_RELEASE}" # package_bindir
                              "${libxcrypt_LIBRARY_TYPE_RELEASE}"
                              "${libxcrypt_IS_HOST_WINDOWS_RELEASE}"
                              libxcrypt_DEPS_TARGET
                              libxcrypt_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libxcrypt"    # package_name
                              "${libxcrypt_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libxcrypt_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET libxcrypt::libxcrypt
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${libxcrypt_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${libxcrypt_LIBRARIES_TARGETS}>
                 )

    if("${libxcrypt_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET libxcrypt::libxcrypt
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     libxcrypt_DEPS_TARGET)
    endif()

    set_property(TARGET libxcrypt::libxcrypt
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${libxcrypt_LINKER_FLAGS_RELEASE}>)
    set_property(TARGET libxcrypt::libxcrypt
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${libxcrypt_INCLUDE_DIRS_RELEASE}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET libxcrypt::libxcrypt
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Release>:${libxcrypt_LIB_DIRS_RELEASE}>)
    set_property(TARGET libxcrypt::libxcrypt
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${libxcrypt_COMPILE_DEFINITIONS_RELEASE}>)
    set_property(TARGET libxcrypt::libxcrypt
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${libxcrypt_COMPILE_OPTIONS_RELEASE}>)

########## For the modules (FindXXX)
set(libxcrypt_LIBRARIES_RELEASE libxcrypt::libxcrypt)
