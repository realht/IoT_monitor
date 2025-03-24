# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/libsystemd-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${libsystemd_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${libsystemd_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET libsystemd::libsystemd)
    add_library(libsystemd::libsystemd INTERFACE IMPORTED)
    message(${libsystemd_MESSAGE_MODE} "Conan: Target declared 'libsystemd::libsystemd'")
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/libsystemd-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()