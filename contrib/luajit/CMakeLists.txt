include(FindPythonInterp)

# Construct list of files whose modification should trigger a rebuild of
# the Python API.
foreach(x ${MONOLITH_SOURCES} ${MONOLITH_HEADERS} ${A5O_PUBLIC_HEADERS})
    if(NOT ${x} MATCHES "^/.*")
        file(RELATIVE_PATH xrel ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/${x})
        list(APPEND SOURCES ${xrel})
    endif()
endforeach()

if(WIN32)
    add_custom_command(
        OUTPUT luajit_protos
        DEPENDS ${SOURCES}
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/python/checkdocs.py
            -c ${CMAKE_C_COMPILER}
            -p luajit_protos
            -b ${PROJECT_BINARY_DIR}
            -s ${PROJECT_SOURCE_DIR}
            -w
        )
else(WIN32)
    add_custom_command(
        OUTPUT luajit_protos
        DEPENDS ${SOURCES}
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/python/checkdocs.py
            -p luajit_protos
            -b ${PROJECT_BINARY_DIR}
            -s ${PROJECT_SOURCE_DIR}
        )
endif(WIN32)

SET(release "")
append_lib_type_suffix(release)
append_lib_linkage_suffix(release)
SET(version "${A5O_SOVERSION}")

add_custom_target(luajit
    ALL
    DEPENDS luajit_protos
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_luajit_ffi.py
            -p luajit_protos
            -o al5_ffi.lua
            -t \"${release}\"
            -v \"${version}\"
    )
