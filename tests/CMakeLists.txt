set(TESTS_EXE_PATH "" CACHE INTERNAL "internal")
set(EXECUTABLE_OUTPUT_PATH ${TESTS_EXE_PATH})

set(TESTS_LIB_PATH "" CACHE INTERNAL "internal")
set(LIBRARY_OUTPUT_PATH ${TESTS_LIB_PATH})

#-----------------------------------------------------------------------------#

macro(test nm)
    add_allegro_executable(${nm} ${nm}.c)
endmacro(test nm)

test(afinfo)
test(akaitest)
test(digitest)
test(filetest)
test(gfxinfo)
test(mathtest)
test(miditest)
test(play)
test(playfli)
test(test)
test(vesainfo)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
