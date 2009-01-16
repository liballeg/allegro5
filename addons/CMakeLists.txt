# FOO_LINK_WITH and SUPPORT_FOO need to be propagated up to the parent scope so
# examples know if they can use the FOO addon, and what to link against.
# FOO_LINK_WITH should be the name of a single target.  The target should
# automatically pull in dependencies so they don't need to be listed.

if(WANT_IIO)
    add_subdirectory(iio)
    set(IIO_LINK_WITH ${IIO_LINK_WITH} PARENT_SCOPE)
    set(SUPPORT_IIO 1 PARENT_SCOPE)

    if(WANT_FONT)
        add_subdirectory(font)
        set(FONT_LINK_WITH ${FONT_LINK_WITH} PARENT_SCOPE)
        set(SUPPORT_FONT 1 PARENT_SCOPE)
    endif(WANT_FONT)
endif(WANT_IIO)

if(WANT_KCM_AUDIO)
    add_subdirectory(kcm_audio)
    if(SUPPORT_KCM_AUDIO)
        set(SUPPORT_KCM_AUDIO 1 PARENT_SCOPE)
        set(AUDIO_LINK_WITH ${AUDIO_LINK_WITH} PARENT_SCOPE)
    endif(SUPPORT_KCM_AUDIO)
endif(WANT_KCM_AUDIO)

if(WANT_ACODEC AND SUPPORT_KCM_AUDIO)
    add_subdirectory(acodec)
    if(SUPPORT_ACODEC)
        set(SUPPORT_ACODEC 1 PARENT_SCOPE)
        set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} PARENT_SCOPE)
    endif(SUPPORT_ACODEC)
endif(WANT_ACODEC AND SUPPORT_KCM_AUDIO)

if(WANT_ICODEC)
    add_subdirectory(icodec)
    if(SUPPORT_ICODEC)
        set(SUPPORT_ICODEC ${SUPPORT_ICODEC} PARENT_SCOPE)
        set(ICODEC_LINK_WITH ${ICODEC_LINK_WITH} PARENT_SCOPE)
    endif(SUPPORT_ICODEC)
endif(WANT_ICODEC)

if(WANT_TTF)
    find_package(Freetype)
    if(FREETYPE_FOUND)
        add_subdirectory(ttf)
        set(SUPPORT_TTF 1 PARENT_SCOPE)
        set(TTF_LINK_WITH ${TTF_LINK_WITH} PARENT_SCOPE)
    else(FREETYPE_FOUND)
        message("WARNING: FreeType not found, disabling support.")
    endif(FREETYPE_FOUND)
endif(WANT_TTF)

if(WANT_COLOR)
    add_subdirectory(color)
    set(SUPPORT_COLOR 1 PARENT_SCOPE)
    set(COLOR_LINK_WITH ${COLOR_LINK_WITH} PARENT_SCOPE)
endif(WANT_COLOR)


if(WANT_MEMFILE)
    add_subdirectory(memfile)
    set(SUPPORT_MEMFILE 1 PARENT_SCOPE)
    set(MEMFILE_LINK_WITH ${MEMFILE_LINK_WITH} PARENT_SCOPE)
endif(WANT_MEMFILE)

if(WANT_PRIMITIVES)
    add_subdirectory(primitives)
    set(SUPPORT_PRIMITIVES 1 PARENT_SCOPE)
    set(PRIMITIVES_LINK_WITH ${PRIMITIVES_LINK_WITH} PARENT_SCOPE)
endif(WANT_PRIMITIVES)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
