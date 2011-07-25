include(FindPythonInterp)

if(WIN32)
    add_custom_command(
        OUTPUT python_protos
        DEPENDS ${ALL_SRC}
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/python/checkdocs.py
            -c ${CMAKE_C_COMPILER}
            -p python_protos
            -b ${CMAKE_BINARY_DIR}
            -s ${CMAKE_SOURCE_DIR}
            -w
        )
else(WIN32)
    add_custom_command(
        OUTPUT python_protos
        DEPENDS ${ALL_SRC}
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/python/checkdocs.py
            -p python_protos
            -b ${CMAKE_BINARY_DIR}
            -s ${CMAKE_SOURCE_DIR}
        )
endif(WIN32)

SET(release "")
append_lib_type_suffix(release)
append_lib_linkage_suffix(release)
SET(version "${ALLEGRO_SOVERSION}")

add_custom_command(
    OUTPUT allegro.py
    DEPENDS python_protos
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/python/generate_python_ctypes.py
        -p python_protos
        -o allegro.py
        -t \"${release}\"
        -v \"${version}\"
    )

add_custom_target(python
    ALL
    DEPENDS allegro.py
    )
