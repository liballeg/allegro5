set(ICODEC_INCLUDE_FILES allegro5/icodec.h)

if(WANT_ICODEC)
    find_path(MAGICK_INCLUDE_DIR Magick++.h)
    find_library(MAGICK_LIBRARY NAMES Magick++)
    mark_as_advanced(MAGICK_INCLUDE_DIR MAGICK_LIBRARY)

    if(MAGICK_INCLUDE_DIR AND MAGICK_LIBRARY)
        set(CAN_MAGICK 1)
    endif(MAGICK_INCLUDE_DIR AND MAGICK_LIBRARY)

    if(CAN_MAGICK)
        set(SUPPORT_MAGICK 1)
        include_directories(${ICODEC_INCLUDE_DIR})
        list(APPEND ICODEC_EXTRA_LIBS Magick++)
        list(APPEND ICODEC_SOURCES icodec.cpp)
    endif(CAN_MAGICK)
endif(WANT_ICODEC)

macro(add_icodec_build nm extra_flags)
    macro(really_add_icodec_build nm target libtype more_extra_flags)
        add_library(${target} ${libtype} ${ICODEC_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nm}
            )

        target_link_libraries(${target} ${LINK_WITH})
        target_link_libraries(${target} ${ICODEC_EXTRA_LIBS})

        install(TARGETS ${target}
                DESTINATION lib
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_icodec_build)

    if(SHARED)
        really_add_icodec_build(${nm} ${nm}_shared SHARED "")
    endif(SHARED)

    if(STATIC)
        really_add_icodec_build(${nm} ${nm}_static STATIC "")
    endif(STATIC)

endmacro(add_icodec_build)

if(GRADE_STANDARD)
    add_icodec_build(a5_icodec
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_icodec_build(a5_icodecd
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_icodec_build(a5_icodecp
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)

if(GRADE_STANDARD)
    set(ICODEC_LIB_TO_LINK a5_icodec CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(ICODEC_LIB_TO_LINK a5_icodecd CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(ICODEC_LIB_TO_LINK a5_icodecp CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(ICODEC_LINK_WITH ${ICODEC_LIB_TO_LINK}_static CACHE INTERNAL "internal")
else(STATIC)
    set(ICODEC_LINK_WITH ${ICODEC_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
endif(STATIC)

set(ICODEC_LINK_WITH ${ICODEC_LINK_WITH} Magick++)


macro(add_icodec_example nm)
    add_executable(${nm} ${EXECUTABLE_TYPE} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK} ${WFLAGS} ${EXE_CFLAGS}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
    target_link_libraries(${nm} ${ICODEC_LINK_WITH})
endmacro(add_icodec_example)

if(SUPPORT_MAGICK)
    add_icodec_example(ex_icodec ex_icodec.c)
endif(SUPPORT_MAGICK)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${ICODEC_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
