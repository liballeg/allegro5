#          ______   ___    ___
#         /\  _  \ /\_ \  /\_ \
#         \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
#          \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
#           \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
#            \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
#             \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
#                                            /\____/
#                                            \_/__/
# 
#       MPW make intented to build allegro.
# 
#       By Ronaldo H. Yamada
#  
#       See readme.txt for copyright information.
# 
# 

#       Use the file install.mac!!

MAKEFILE     = allegro.make
¥MondoBuild¥ = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = -i :include:mac:
Sym¥PPC      =  
ObjDir¥PPC   = :obj:mpw:alld:
SourceDir    = :src:

PPCCOptions  = {Includes} {Sym¥PPC} -w 35 -w 29

Objects¥PPC  = ¶
		"{ObjDir¥PPC}allegro.c.x" ¶
		"{ObjDir¥PPC}blit.c.x" ¶
		"{ObjDir¥PPC}bmp.c.x" ¶
		"{ObjDir¥PPC}clip3d.c.x" ¶
		"{ObjDir¥PPC}clip3df.c.x" ¶
		"{ObjDir¥PPC}colblend.c.x" ¶
		"{ObjDir¥PPC}color.c.x" ¶
		"{ObjDir¥PPC}config.c.x" ¶
		"{ObjDir¥PPC}datafile.c.x" ¶
		"{ObjDir¥PPC}dataregi.c.x" ¶
		"{ObjDir¥PPC}digmid.c.x" ¶
		"{ObjDir¥PPC}dispsw.c.x" ¶
		"{ObjDir¥PPC}dither.c.x" ¶
		"{ObjDir¥PPC}file.c.x" ¶
		"{ObjDir¥PPC}fli.c.x" ¶
		"{ObjDir¥PPC}flood.c.x" ¶
		"{ObjDir¥PPC}fsel.c.x" ¶
		"{ObjDir¥PPC}gfx.c.x" ¶
		"{ObjDir¥PPC}glyph.c.x" ¶
		"{ObjDir¥PPC}graphics.c.x" ¶
		"{ObjDir¥PPC}gsprite.c.x" ¶
		"{ObjDir¥PPC}gui.c.x" ¶
		"{ObjDir¥PPC}guiproc.c.x" ¶
		"{ObjDir¥PPC}inline.c.x" ¶
		"{ObjDir¥PPC}joystick.c.x" ¶
		"{ObjDir¥PPC}keyboard.c.x" ¶
		"{ObjDir¥PPC}lbm.c.x" ¶
		"{ObjDir¥PPC}libc.c.x" ¶
		"{ObjDir¥PPC}math.c.x" ¶
		"{ObjDir¥PPC}math3d.c.x" ¶
		"{ObjDir¥PPC}midi.c.x" ¶
		"{ObjDir¥PPC}mixer.c.x" ¶
		"{ObjDir¥PPC}modesel.c.x" ¶
		"{ObjDir¥PPC}mouse.c.x" ¶
		"{ObjDir¥PPC}pcx.c.x" ¶
		"{ObjDir¥PPC}poly3d.c.x" ¶
		"{ObjDir¥PPC}polygon.c.x" ¶
		"{ObjDir¥PPC}quantize.c.x" ¶
		"{ObjDir¥PPC}quat.c.x" ¶
		"{ObjDir¥PPC}readbmp.c.x" ¶
		"{ObjDir¥PPC}rle.c.x" ¶
		"{ObjDir¥PPC}rotate.c.x" ¶
		"{ObjDir¥PPC}sound.c.x" ¶
		"{ObjDir¥PPC}spline.c.x" ¶
		"{ObjDir¥PPC}stream.c.x" ¶
		"{ObjDir¥PPC}text.c.x" ¶
		"{ObjDir¥PPC}tga.c.x" ¶
		"{ObjDir¥PPC}timer.c.x" ¶
		"{ObjDir¥PPC}unicode.c.x" ¶
		"{ObjDir¥PPC}vtable.c.x" ¶
		"{ObjDir¥PPC}vtable15.c.x" ¶
		"{ObjDir¥PPC}vtable16.c.x" ¶
		"{ObjDir¥PPC}vtable24.c.x" ¶
		"{ObjDir¥PPC}vtable32.c.x" ¶
		"{ObjDir¥PPC}vtable8.c.x" ¶
		"{ObjDir¥PPC}cblit16.c.x" ¶
		"{ObjDir¥PPC}cblit24.c.x" ¶
		"{ObjDir¥PPC}cblit32.c.x" ¶
		"{ObjDir¥PPC}cblit8.c.x" ¶
		"{ObjDir¥PPC}ccpu.c.x" ¶
		"{ObjDir¥PPC}ccsprite.c.x" ¶
		"{ObjDir¥PPC}cgfx15.c.x" ¶
		"{ObjDir¥PPC}cgfx16.c.x" ¶
		"{ObjDir¥PPC}cgfx24.c.x" ¶
		"{ObjDir¥PPC}cgfx32.c.x" ¶
		"{ObjDir¥PPC}cgfx8.c.x" ¶
		"{ObjDir¥PPC}cmisc.c.x" ¶
		"{ObjDir¥PPC}cscan15.c.x" ¶
		"{ObjDir¥PPC}cscan16.c.x" ¶
		"{ObjDir¥PPC}cscan24.c.x" ¶
		"{ObjDir¥PPC}cscan32.c.x" ¶
		"{ObjDir¥PPC}cscan8.c.x" ¶
		"{ObjDir¥PPC}cspr15.c.x" ¶
		"{ObjDir¥PPC}cspr16.c.x" ¶
		"{ObjDir¥PPC}cspr24.c.x" ¶
		"{ObjDir¥PPC}cspr32.c.x" ¶
		"{ObjDir¥PPC}cspr8.c.x" ¶
		"{ObjDir¥PPC}cstretch.c.x" ¶
		"{ObjDir¥PPC}macallegro.c.x" ¶
		"{ObjDir¥PPC}BlitPixie.s.x"


allegro ÄÄ {¥MondoBuild¥} {Objects¥PPC}
	PPCLink ¶
		-xm l¶
		-o "{ObjDir¥PPC}{Targ}.x" -sym big ¶
		{Objects¥PPC} ¶
		-mf ¶
		-t 'XCOF' ¶
		-c 'MPS '

"{ObjDir¥PPC}allegro.c.x" Ä {¥MondoBuild¥} {SourceDir}allegro.c
	{PPCC} {SourceDir}allegro.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}blit.c.x" Ä {¥MondoBuild¥} {SourceDir}blit.c
	{PPCC} {SourceDir}blit.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}bmp.c.x" Ä {¥MondoBuild¥} {SourceDir}bmp.c
	{PPCC} {SourceDir}bmp.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}clip3d.c.x" Ä {¥MondoBuild¥} {SourceDir}clip3d.c
	{PPCC} {SourceDir}clip3d.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}clip3df.c.x" Ä {¥MondoBuild¥} {SourceDir}clip3df.c
	{PPCC} {SourceDir}clip3df.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}colblend.c.x" Ä {¥MondoBuild¥} {SourceDir}colblend.c
	{PPCC} {SourceDir}colblend.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}color.c.x" Ä {¥MondoBuild¥} {SourceDir}color.c
	{PPCC} {SourceDir}color.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}config.c.x" Ä {¥MondoBuild¥} {SourceDir}config.c
	{PPCC} {SourceDir}config.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}datafile.c.x" Ä {¥MondoBuild¥} {SourceDir}datafile.c
	{PPCC} {SourceDir}datafile.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}dataregi.c.x" Ä {¥MondoBuild¥} {SourceDir}dataregi.c
	{PPCC} {SourceDir}dataregi.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}digmid.c.x" Ä {¥MondoBuild¥} {SourceDir}digmid.c
	{PPCC} {SourceDir}digmid.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}dispsw.c.x" Ä {¥MondoBuild¥} {SourceDir}dispsw.c
	{PPCC} {SourceDir}dispsw.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}dither.c.x" Ä {¥MondoBuild¥} {SourceDir}dither.c
	{PPCC} {SourceDir}dither.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}file.c.x" Ä {¥MondoBuild¥} {SourceDir}file.c
	{PPCC} {SourceDir}file.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}fli.c.x" Ä {¥MondoBuild¥} {SourceDir}fli.c
	{PPCC} {SourceDir}fli.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}flood.c.x" Ä {¥MondoBuild¥} {SourceDir}flood.c
	{PPCC} {SourceDir}flood.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}fsel.c.x" Ä {¥MondoBuild¥} {SourceDir}fsel.c
	{PPCC} {SourceDir}fsel.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}gfx.c.x" Ä {¥MondoBuild¥} {SourceDir}gfx.c
	{PPCC} {SourceDir}gfx.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}glyph.c.x" Ä {¥MondoBuild¥} {SourceDir}glyph.c
	{PPCC} {SourceDir}glyph.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}graphics.c.x" Ä {¥MondoBuild¥} {SourceDir}graphics.c
	{PPCC} {SourceDir}graphics.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}gsprite.c.x" Ä {¥MondoBuild¥} {SourceDir}gsprite.c
	{PPCC} {SourceDir}gsprite.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}gui.c.x" Ä {¥MondoBuild¥} {SourceDir}gui.c
	{PPCC} {SourceDir}gui.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}guiproc.c.x" Ä {¥MondoBuild¥} {SourceDir}guiproc.c
	{PPCC} {SourceDir}guiproc.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}inline.c.x" Ä {¥MondoBuild¥} {SourceDir}inline.c
	{PPCC} {SourceDir}inline.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}joystick.c.x" Ä {¥MondoBuild¥} {SourceDir}joystick.c
	{PPCC} {SourceDir}joystick.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}keyboard.c.x" Ä {¥MondoBuild¥} {SourceDir}keyboard.c
	{PPCC} {SourceDir}keyboard.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}lbm.c.x" Ä {¥MondoBuild¥} {SourceDir}lbm.c
	{PPCC} {SourceDir}lbm.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}libc.c.x" Ä {¥MondoBuild¥} {SourceDir}libc.c
	{PPCC} {SourceDir}libc.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}math.c.x" Ä {¥MondoBuild¥} {SourceDir}math.c
	{PPCC} {SourceDir}math.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}math3d.c.x" Ä {¥MondoBuild¥} {SourceDir}math3d.c
	{PPCC} {SourceDir}math3d.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}midi.c.x" Ä {¥MondoBuild¥} {SourceDir}midi.c
	{PPCC} {SourceDir}midi.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}mixer.c.x" Ä {¥MondoBuild¥} {SourceDir}mixer.c
	{PPCC} {SourceDir}mixer.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}modesel.c.x" Ä {¥MondoBuild¥} {SourceDir}modesel.c
	{PPCC} {SourceDir}modesel.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}mouse.c.x" Ä {¥MondoBuild¥} {SourceDir}mouse.c
	{PPCC} {SourceDir}mouse.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}pcx.c.x" Ä {¥MondoBuild¥} {SourceDir}pcx.c
	{PPCC} {SourceDir}pcx.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}poly3d.c.x" Ä {¥MondoBuild¥} {SourceDir}poly3d.c
	{PPCC} {SourceDir}poly3d.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}polygon.c.x" Ä {¥MondoBuild¥} {SourceDir}polygon.c
	{PPCC} {SourceDir}polygon.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}quantize.c.x" Ä {¥MondoBuild¥} {SourceDir}quantize.c
	{PPCC} {SourceDir}quantize.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}quat.c.x" Ä {¥MondoBuild¥} {SourceDir}quat.c
	{PPCC} {SourceDir}quat.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}readbmp.c.x" Ä {¥MondoBuild¥} {SourceDir}readbmp.c
	{PPCC} {SourceDir}readbmp.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}rle.c.x" Ä {¥MondoBuild¥} {SourceDir}rle.c
	{PPCC} {SourceDir}rle.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}rotate.c.x" Ä {¥MondoBuild¥} {SourceDir}rotate.c
	{PPCC} {SourceDir}rotate.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}sound.c.x" Ä {¥MondoBuild¥} {SourceDir}sound.c
	{PPCC} {SourceDir}sound.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}spline.c.x" Ä {¥MondoBuild¥} {SourceDir}spline.c
	{PPCC} {SourceDir}spline.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}stream.c.x" Ä {¥MondoBuild¥} {SourceDir}stream.c
	{PPCC} {SourceDir}stream.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}text.c.x" Ä {¥MondoBuild¥} {SourceDir}text.c
	{PPCC} {SourceDir}text.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}tga.c.x" Ä {¥MondoBuild¥} {SourceDir}tga.c
	{PPCC} {SourceDir}tga.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}timer.c.x" Ä {¥MondoBuild¥} {SourceDir}timer.c
	{PPCC} {SourceDir}timer.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}unicode.c.x" Ä {¥MondoBuild¥} {SourceDir}unicode.c
	{PPCC} {SourceDir}unicode.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}vtable.c.x" Ä {¥MondoBuild¥} {SourceDir}vtable.c
	{PPCC} {SourceDir}vtable.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}vtable15.c.x" Ä {¥MondoBuild¥} {SourceDir}vtable15.c
	{PPCC} {SourceDir}vtable15.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}vtable16.c.x" Ä {¥MondoBuild¥} {SourceDir}vtable16.c
	{PPCC} {SourceDir}vtable16.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}vtable24.c.x" Ä {¥MondoBuild¥} {SourceDir}vtable24.c
	{PPCC} {SourceDir}vtable24.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}vtable32.c.x" Ä {¥MondoBuild¥} {SourceDir}vtable32.c
	{PPCC} {SourceDir}vtable32.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}vtable8.c.x" Ä {¥MondoBuild¥} {SourceDir}vtable8.c
	{PPCC} {SourceDir}vtable8.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cblit16.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cblit16.c
	{PPCC} {SourceDir}c:cblit16.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cblit24.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cblit24.c
	{PPCC} {SourceDir}c:cblit24.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cblit32.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cblit32.c
	{PPCC} {SourceDir}c:cblit32.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cblit8.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cblit8.c
	{PPCC} {SourceDir}c:cblit8.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}ccpu.c.x" Ä {¥MondoBuild¥} {SourceDir}c:ccpu.c
	{PPCC} {SourceDir}c:ccpu.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}ccsprite.c.x" Ä {¥MondoBuild¥} {SourceDir}c:ccsprite.c
	{PPCC} {SourceDir}c:ccsprite.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cgfx15.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cgfx15.c
	{PPCC} {SourceDir}c:cgfx15.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cgfx16.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cgfx16.c
	{PPCC} {SourceDir}c:cgfx16.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cgfx24.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cgfx24.c
	{PPCC} {SourceDir}c:cgfx24.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cgfx32.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cgfx32.c
	{PPCC} {SourceDir}c:cgfx32.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cgfx8.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cgfx8.c
	{PPCC} {SourceDir}c:cgfx8.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cmisc.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cmisc.c
	{PPCC} {SourceDir}c:cmisc.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cscan15.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cscan15.c
	{PPCC} {SourceDir}c:cscan15.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cscan16.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cscan16.c
	{PPCC} {SourceDir}c:cscan16.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cscan24.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cscan24.c
	{PPCC} {SourceDir}c:cscan24.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cscan32.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cscan32.c
	{PPCC} {SourceDir}c:cscan32.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cscan8.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cscan8.c
	{PPCC} {SourceDir}c:cscan8.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cspr15.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cspr15.c
	{PPCC} {SourceDir}c:cspr15.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cspr16.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cspr16.c
	{PPCC} {SourceDir}c:cspr16.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cspr24.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cspr24.c
	{PPCC} {SourceDir}c:cspr24.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cspr32.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cspr32.c
	{PPCC} {SourceDir}c:cspr32.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cspr8.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cspr8.c
	{PPCC} {SourceDir}c:cspr8.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}cstretch.c.x" Ä {¥MondoBuild¥} {SourceDir}c:cstretch.c
	{PPCC} {SourceDir}c:cstretch.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}macallegro.c.x" Ä {¥MondoBuild¥} {SourceDir}mac:macallegro.c
	{PPCC} {SourceDir}mac:macallegro.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}BlitPixie.s.x" Ä {¥MondoBuild¥} {SourceDir}mac:BlitPixie.s
	{PPCAsm} {SourceDir}mac:BlitPixie.s -o {Targ} 
