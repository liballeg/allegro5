#
# to use this file replace "template" to name of your source
#

MAKEFILE     = template.make
¥MondoBuild¥ = {MAKEFILE}  				# Make blank to avoid rebuilds when makefile is modified
Includes     = -i "{Allegro}"			# the Allegro include location
Sym¥PPC      = -sym on 	   				# debug is on (disable optimizations)
ObjDir¥PPC   =							# if you prefer objs in another Folder

PPCCOptions  = {Includes} {Sym¥PPC} 

# the list of objs
Objects¥PPC  = ¶
		"{ObjDir¥PPC}template.c.x" ¶
		"{ObjDir¥PPC}template2.c.x"
		

#SHOULD ALWAYS PUT OPTION -main MacEntry
#SHOULD ALWAYS Link with {Allegro}allegro.x
template ÄÄ {¥MondoBuild¥} {Objects¥PPC}
	PPCLink ¶
		-o {Targ} -main MacEntry {Sym¥PPC} ¶
		{Objects¥PPC} ¶
		-t 'APPL' ¶
		-c '????' ¶
		-mf ¶
		"{SharedLibraries}InterfaceLib" ¶
		"{SharedLibraries}StdCLib" ¶
		"{SharedLibraries}MathLib" ¶
		"{SharedLibraries}DrawSprocketLib" ¶
		"{Allegro}allegro.x" ¶
		"{PPCLibraries}StdCRuntime.o" ¶
		"{PPCLibraries}PPCCRuntime.o" ¶
		"{PPCLibraries}PPCToolLibs.o"

#APPEND the allegro specific resource
template ÄÄ {¥MondoBuild¥} {Allegro}allegro.r
	Rez {Allegro}allegro.r -o {Targ} {Includes} -append

"{ObjDir¥PPC}template.c.x" Ä {¥MondoBuild¥} template.c
	{PPCC} template.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}template2.c.x" Ä {¥MondoBuild¥} template2.c
	{PPCC} template2.c -o {Targ} {PPCCOptions}

