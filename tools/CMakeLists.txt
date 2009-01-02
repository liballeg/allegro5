#
# Other tools
#

macro(tool nm)
    add_our_executable(${nm} ${ALLEGRO_LINK_WITH})
    install(TARGETS ${nm}
            DESTINATION bin
            )
endmacro(tool nm)

tool(colormap)
tool(pack)
tool(rgbmap)
tool(textconv)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
