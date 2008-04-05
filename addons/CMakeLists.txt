if(WANT_FONT)
    add_subdirectory(font)
endif(WANT_FONT)

if(WANT_AUDIO)
    add_subdirectory(audio)
endif(WANT_AUDIO)

if(WANT_ACODEC)
    add_subdirectory(acodec)
endif(WANT_ACODEC)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

