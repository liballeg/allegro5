if(WANT_FONT)
    add_subdirectory(font)
endif(WANT_FONT)

if(WANT_AUDIO)
    add_subdirectory(audio)

    if(WANT_ACODEC)
        add_subdirectory(acodec)
    endif(WANT_ACODEC)
endif(WANT_AUDIO)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

