set(IIO_SOURCES bmp.c iio.c pcx.c tga.c)
set(IIO_INCLUDE_FILES allegro5/a5_iio.h)

if(WANT_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        set(ALLEGRO_CFG_IIO_HAVE_PNG 1)
        list(APPEND IIO_SOURCES png.c)
        set(IIO_LIBRARIES ${PNG_LIBRARIES})
        set(IIO_DEFINES ${PNG_DEFINITIONS})
        include_directories(${PNG_INCLUDE_DIR})
    endif(PNG_FOUND)
endif(WANT_PNG)

if(WANT_JPG)
    find_package(JPEG)
    if(JPEG_FOUND)
        set(ALLEGRO_CFG_IIO_HAVE_JPG 1)
        list(APPEND IIO_SOURCES jpg.c)
        list(APPEND IIO_LIBRARIES ${JPEG_LIBRARIES})
        list(APPEND IIO_DEFINES ${JPEG_DEFINITIONS})
        include_directories(${JPEG_INCLUDE_DIR})
    endif(JPEG_FOUND)
endif(WANT_JPG)

configure_file(
    allegro5/internal/aintern_iio_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_iio_cfg.h
    )

macro(add_iio_build nm extra_flags)
    macro(really_add_iio_build nm2 target libtype more_extra_flags)
        add_library(${target} ${libtype} ${IIO_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${IIO_DEFINES} ${LIBRARY_CFLAGS} -DA5_IIO_SRC"
            OUTPUT_NAME ${nm2}
            )
        
        target_link_libraries(${target} ${IIO_LIBRARIES})

        install(TARGETS ${target}
                DESTINATION "lib${LIB_SUFFIX}"
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
        really_add_iio_build(${nm}-static ${nm}_static STATIC "-DALLEGRO_STATICLINK")
    endif(STATIC)

endmacro(add_iio_build)

if(GRADE_STANDARD)
    add_iio_build(a5_iio
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_iio_build(a5_iio-debug
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_iio_build(a5_iio-profile
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)


if(GRADE_STANDARD)
    set(IIO_LIB_TO_LINK a5_iio CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(IIO_LIB_TO_LINK a5_iio-debug CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(IIO_LIB_TO_LINK a5_iio-profile CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(IIO_LINK_WITH ${IIO_LIB_TO_LINK}_static)
    set(IIO_LINK_WITH ${IIO_LIB_TO_LINK}_static PARENT_SCOPE)
else(STATIC)
    set(IIO_LINK_WITH ${IIO_LIB_TO_LINK}_shared)
    set(IIO_LINK_WITH ${IIO_LIB_TO_LINK}_shared PARENT_SCOPE)
endif(STATIC)

if(SHARED)
    set(IIO_ADDONS_LINK_WITH ${IIO_LIB_TO_LINK}_shared PARENT_SCOPE)
endif(SHARED)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${IIO_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

