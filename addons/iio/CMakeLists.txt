set(IIO_SOURCES bmp.c iio.c pcx.c tga.c)
set(IIO_INCLUDE_FILES allegro5/a5_iio.h)

macro(add_iio_build nm extra_flags)
    macro(really_add_iio_build nm2 target libtype more_extra_flags)
        add_library(${target} ${libtype} ${IIO_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nm2}
            )

        install(TARGETS ${target}
                DESTINATION lib
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_iio_build)

    if(SHARED)
        really_add_iio_build(${nm} ${nm}_shared SHARED "")
        target_link_libraries(${nm}_shared ${ADDONS_LINK_WITH})
    endif(SHARED)

    if(STATIC)
        really_add_iio_build(${nm}_s ${nm}_static STATIC "-DALLEGRO_STATICLINK")
    endif(STATIC)

endmacro(add_iio_build)

if(GRADE_STANDARD)
    add_iio_build(a5_iio
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_iio_build(a5_iiod
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_iio_build(a5_iiop
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)


if(GRADE_STANDARD)
    set(IIO_LIB_TO_LINK a5_iio CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(IIO_LIB_TO_LINK a5_iiod CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(IIO_LIB_TO_LINK a5_iiop CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(IIO_LINK_WITH ${IIO_LIB_TO_LINK}_static)
    set(IIO_LINK_WITH ${IIO_LIB_TO_LINK}_static PARENT_SCOPE)
else(STATIC)
    set(IIO_LINK_WITH ${IIO_LIB_TO_LINK}_shared)
    set(IIO_LINK_WITH ${IIO_LIB_TO_LINK}_shared PARENT_SCOPE)
endif(STATIC)

macro(add_iio_example nm)
    add_executable(${nm} ${EXECUTABLE_TYPE} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK} ${WFLAGS} ${EXE_CFLAGS}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
    target_link_libraries(${nm} ${IIO_LINK_WITH})
endmacro(add_iio_example)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${FONT_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

