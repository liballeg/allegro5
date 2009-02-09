#
MAKEFILE     = test.make
¥MondoBuild¥ = {MAKEFILE}  				# Make blank to avoid rebuilds when makefile is modified
Includes     = -i "{Allegro}"			# the Allegro include location
Sym¥PPC      = -sym on 	   				# debug is on (disable optimizations)
ObjDir¥PPC   =							# if you prefer objs in another Folder

PPCCOptions  = {Includes} {Sym¥PPC} 

# the list of objs
Objects¥PPC  = ¶
		"{ObjDir¥PPC}test.c.x"
		

#SHOULD ALWAYS PUT OPTION -main MacEntry
#SHOULD ALWAYS Link with {Allegro}allegro.x
test ÄÄ {¥MondoBuild¥} {Objects¥PPC}
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
test ÄÄ {¥MondoBuild¥} {Allegro}allegro.r
	Rez {Allegro}allegro.r -o {Targ} {Includes} -append

"{ObjDir¥PPC}test.c.x" Ä {¥MondoBuild¥} "::tests:test.c"
	{PPCC} "::tests:test.c" -o {Targ} {PPCCOptions}

