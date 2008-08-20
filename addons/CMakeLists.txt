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

    if(WANT_ACODEC)
        add_subdirectory(acodec)
    endif(WANT_ACODEC)
endif(WANT_AUDIO)

if(WANT_ICODEC)
    add_subdirectory(icodec)
endif(WANT_ICODEC)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

