########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(libcap_FIND_QUIETLY)
    set(libcap_MESSAGE_MODE VERBOSE)
else()
    set(libcap_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libcapTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${libcap_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(libcap_VERSION_STRING "2.69")
set(libcap_INCLUDE_DIRS ${libcap_INCLUDE_DIRS_RELEASE} )
set(libcap_INCLUDE_DIR ${libcap_INCLUDE_DIRS_RELEASE} )
set(libcap_LIBRARIES ${libcap_LIBRARIES_RELEASE} )
set(libcap_DEFINITIONS ${libcap_DEFINITIONS_RELEASE} )


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${libcap_BUILD_MODULES_PATHS_RELEASE} )
    message(${libcap_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


