include(FindPythonInterp)

set(ALLEGRO_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/protos.txt)
set(LUAJIT_OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/al5_ffi.lua)

add_custom_command(
    OUTPUT ${ALLEGRO_PROTOS}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/python/checkdocs.py
            -p ${ALLEGRO_PROTOS}
            -b ${PROJECT_BINARY_DIR}
            -s ${PROJECT_SOURCE_DIR}
    )

add_custom_target(contrib_luajit
    DEPENDS ${ALLEGRO_PROTOS}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_luajit_ffi.py
            -p ${ALLEGRO_PROTOS}
            -o ${LUAJIT_OUTPUT_FILE}
    )
