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

if(WANT_AUDIO)
    add_subdirectory(audio)
    if(SUPPORT_AUDIO)
        set(SUPPORT_AUDIO 1 PARENT_SCOPE)
        set(AUDIO_LINK_WITH ${AUDIO_LINK_WITH} PARENT_SCOPE)
    endif(SUPPORT_AUDIO)
endif(WANT_AUDIO)

if(SUPPORT_AUDIO)
    add_subdirectory(acodec)
    if(SUPPORT_FLAC)
        set(SUPPORT_FLAC 1 PARENT_SCOPE)
        set(FLAC_LINK_WITH ${FLAC_LINK_WITH} PARENT_SCOPE)
    endif(SUPPORT_FLAC)
    if(SUPPORT_VORBIS)
        set(SUPPORT_VORBIS 1 PARENT_SCOPE)
        set(VORBIS_LINK_WITH ${VORBIS_LINK_WITH} PARENT_SCOPE)
    endif(SUPPORT_VORBIS)
endif(SUPPORT_AUDIO)

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

if(WANT_PHYSFS)
    find_package(PhysFS)
    mark_as_advanced(PHYSFS_INCLUDE_DIR)
    mark_as_advanced(PHYSFS_LIBRARY)
    if(PHYSFS_FOUND)
        add_subdirectory(physfs)
        set(SUPPORT_PHYSFS 1 PARENT_SCOPE)
        set(PHYSFS_LINK_WITH ${PHYSFS_LINK_WITH} PARENT_SCOPE)
    endif(PHYSFS_FOUND)
endif(WANT_PHYSFS)

if(WANT_PRIMITIVES)
    add_subdirectory(primitives)
    set(SUPPORT_PRIMITIVES 1 PARENT_SCOPE)
    set(PRIMITIVES_LINK_WITH ${PRIMITIVES_LINK_WITH} PARENT_SCOPE)
endif(WANT_PRIMITIVES)

if(WANT_NATIVE_DIALOG)
    add_subdirectory(native_dialog)
    set(SUPPORT_NATIVE_DIALOG ${SUPPORT_NATIVE_DIALOG} PARENT_SCOPE)
    set(NATIVE_DIALOG_LINK_WITH ${NATIVE_DIALOG_LINK_WITH} PARENT_SCOPE)
endif(WANT_NATIVE_DIALOG)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
