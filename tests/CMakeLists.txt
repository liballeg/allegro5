macro(test nm)
    add_our_executable(${nm} ${ALLEGRO_LINK_WITH})
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
