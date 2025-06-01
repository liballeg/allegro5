find_package(Python 3.8 REQUIRED COMPONENTS Interpreter)

# Construct list of files whose modification should trigger a rebuild of
# the Python API.
foreach(x ${MONOLITH_SOURCES} ${MONOLITH_HEADERS} ${ALLEGRO_PUBLIC_HEADERS})
    if(NOT ${x} MATCHES "^/.*")
        file(RELATIVE_PATH xrel ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/${x})
        list(APPEND SOURCES ${xrel})
    endif()
endforeach()

if(WIN32)
    add_custom_command(
        OUTPUT python_protos
        DEPENDS ${SOURCES}
        COMMAND Python::Interpreter ${PROJECT_SOURCE_DIR}/python/checkdocs.py
            -c ${CMAKE_C_COMPILER}
            -p python_protos
            -b ${PROJECT_BINARY_DIR}
            -s ${PROJECT_SOURCE_DIR}
            -w
        )
else(WIN32)
    add_custom_command(
        OUTPUT python_protos
        DEPENDS ${SOURCES}
        COMMAND Python::Interpreter ${PROJECT_SOURCE_DIR}/python/checkdocs.py
            -p python_protos
            -b ${PROJECT_BINARY_DIR}
            -s ${PROJECT_SOURCE_DIR}
        )
endif(WIN32)

SET(release "")
append_lib_type_suffix(release)
append_lib_linkage_suffix(release)
SET(version "${ALLEGRO_SOVERSION}")

add_custom_command(
    OUTPUT allegro.py
    DEPENDS python_protos
    COMMAND Python::Interpreter ${PROJECT_SOURCE_DIR}/python/generate_python_ctypes.py
        -p python_protos
        -o allegro.py
        -t \"${release}\"
        -v \"${version}\"
    )

add_custom_target(python
    ALL
    DEPENDS allegro.py
    )
