########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(libsystemd_FIND_QUIETLY)
    set(libsystemd_MESSAGE_MODE VERBOSE)
else()
    set(libsystemd_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libsystemdTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${libsystemd_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(libsystemd_VERSION_STRING "255")
set(libsystemd_INCLUDE_DIRS ${libsystemd_INCLUDE_DIRS_RELEASE} )
set(libsystemd_INCLUDE_DIR ${libsystemd_INCLUDE_DIRS_RELEASE} )
set(libsystemd_LIBRARIES ${libsystemd_LIBRARIES_RELEASE} )
set(libsystemd_DEFINITIONS ${libsystemd_DEFINITIONS_RELEASE} )


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${libsystemd_BUILD_MODULES_PATHS_RELEASE} )
    message(${libsystemd_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


