find_package(Python 3.8 REQUIRED COMPONENTS Interpreter)

# Construct list of files whose modification should trigger a rebuild of
# the Python API. We presume that generated files are invariant.
# Otherwise the second loop would yield incorrect paths
# thus increasing the build tool noise.
set(aph ${ALLEGRO_PUBLIC_HEADERS})
foreach(genfile ${ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED})
    list(REMOVE_ITEM aph ${PROJECT_BINARY_DIR}/${genfile})
endforeach(genfile)

foreach(x ${MONOLITH_HEADERS} ${aph})
    if(NOT ${x} MATCHES "^/.*")
        file(RELATIVE_PATH xrel ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/${x})
        list(APPEND SOURCES ${xrel})
    endif()
endforeach()

if(MSVC)
    # We do really need any gcc preprocessor for now as we rely on
    # defines pass-through with -dD that cl.exe lacks.
    # Let's check likely locations.
    find_program(GCC_EXE NAMES gcc.exe PATHS C:/msys64 C:/MinGW C:/MinGW64
        PATH_SUFFIXES bin usr/bin ucrt64/bin mingw64/bin)
    get_filename_component(GCC_PATH ${GCC_EXE} DIRECTORY)
    add_custom_command(
        OUTPUT python_protos
        DEPENDS ${SOURCES}
        COMMAND ${CMAKE_COMMAND} -E env "PATH=$ENV{PATH};${GCC_PATH}"
            ${Python_EXECUTABLE} ${PROJECT_SOURCE_DIR}/python/checkdocs.py
            -c ${GCC_EXE}
            -p python_protos
            -b ${PROJECT_BINARY_DIR}
            -s ${PROJECT_SOURCE_DIR}
            -w
        )
else(MSVC)
    add_custom_command(
        OUTPUT python_protos
        DEPENDS ${SOURCES}
        COMMAND Python::Interpreter ${PROJECT_SOURCE_DIR}/python/checkdocs.py
            -c ${CMAKE_C_COMPILER}
            -p python_protos
            -b ${PROJECT_BINARY_DIR}
            -s ${PROJECT_SOURCE_DIR}
            $<$<BOOL:WIN32>:-w>
        )
endif(MSVC)

SET(release "")
append_lib_type_suffix(release)
append_lib_linkage_suffix(release)
SET(version "${ALLEGRO_SOVERSION}")

if(MSVC)
    # MSBuild will fail if there is "error" in the output
    add_custom_command(
        OUTPUT allegro.py
        DEPENDS python_protos
        COMMAND pwsh -c "${Python_EXECUTABLE} ${PROJECT_SOURCE_DIR}/python/generate_python_ctypes.py \
-p python_protos -o allegro.py -t '${release}' -v '${version}' 2>&1 | ForEach-Object { $_ -replace 'error', 'err0r' }" VERBATIM
        )
else(MSVC)
    add_custom_command(
        OUTPUT allegro.py
        DEPENDS python_protos
        COMMAND Python::Interpreter ${PROJECT_SOURCE_DIR}/python/generate_python_ctypes.py
            -p python_protos
            -o allegro.py
            -t \"${release}\"
            -v \"${version}\"
        )
endif(MSVC)

add_custom_target(python
    ALL
    DEPENDS allegro.py
    )
