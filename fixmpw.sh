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
#       MPW Script install allegro
# 
#       By Ronaldo H. Yamada
#  
#       See readme.txt for copyright information.
#       See readme.mpw for information about use of this file
#

Set savedir `Directory`

Echo {0} --- preparing install 

If "{0}" =~ /(Å:)¨1readme.mpw/
	SetDirectory {¨1}
End

If "{Allegro}"!="{Libraries}::Allegro:"
	Set -e Allegro {Libraries}::Allegro:
	Echo "### Allegro var not defined correctly"
	Echo "### If required quit MPW after install"
End
If !`Exists "{Allegro}"`
	NewFolder "{Allegro}"	
End
If !`Exists ":obj:"`
	NewFolder ":obj:"	
End
If !`Exists ":obj:mpw:"`
	NewFolder ":obj:mpw:"
End
If !`Exists ":obj:mpw:alld:"`
	NewFolder ":obj:mpw:alld:"
End

#change creator to MPW Shell
set f "`Files -r -f -t 'TEXT' -c 'CWIE' -o`"
if "{f}"!=""
	SetFile -c 'MPS ' {f}
end
unset f

If `Exists "makefile.mpw"`
	Echo "analyzing dependencys"
	make -f makefile.mpw {Parameters} > ":obj:mpw:alld:makefile.dep"
	Echo "building sources"
	Echo "this can take several minutes..."
	":obj:mpw:alld:makefile.dep"
Else
	Echo "makefile.mpw not found"
End

Echo "### Done"

SetDirectory "{savedir}"
