#
# sample make for ::examples:exhello.c

MAKEFILE     = exhello.make
¥MondoBuild¥ = {MAKEFILE}  			# Make blank to avoid rebuilds when makefile is modified
Includes     = -i "{Allegro}"		# the Allegro include location
Sym¥PPC      = -sym on 	   			# debug is on (disable optimizations)
SrcDir		 = ::examples:			# you can put here the src dir
ObjDir¥PPC   = :obj:				# if you prefer objs in another Folder
PPCCOptions  = {Includes} {Sym¥PPC} 


Objects¥PPC  = ¶
		"{ObjDir¥PPC}exhello.c.x" ¶


exhello ÄÄ {¥MondoBuild¥} {Objects¥PPC}
	PPCLink ¶
		-o {Targ} -main MacEntry {Sym¥PPC} ¶
		{Objects¥PPC} ¶
		-t 'APPL' ¶
		-c '????' ¶
		-mf ¶
		"{PPCLibraries}PPCSIOW.o" ¶
		"{SharedLibraries}InterfaceLib" ¶
		"{SharedLibraries}StdCLib" ¶
		"{SharedLibraries}MathLib" ¶
		"{SharedLibraries}DrawSprocketLib" ¶
		"{Allegro}allegro.x" ¶
		"{PPCLibraries}StdCRuntime.o" ¶
		"{PPCLibraries}PPCCRuntime.o" ¶
		"{PPCLibraries}PPCToolLibs.o"

exhello ÄÄ {¥MondoBuild¥} {Allegro}allegro.r
	Rez {Allegro}allegro.r -o {Targ} {Includes} -append

exhello ÄÄ "{RIncludes}"SIOW.r
	Rez -a "{RIncludes}"SIOW.r -o {Targ}


"{ObjDir¥PPC}exhello.c.x" Ä {¥MondoBuild¥} "{SrcDir}exhello.c"
	{PPCC} "{SrcDir}exhello.c" -o {Targ} {PPCCOptions}

