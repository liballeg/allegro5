# Microsoft Developer Studio Project File - Name="allegrogl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=allegrogl - Win32 Static Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "allegrogl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "allegrogl.mak" CFG="allegrogl - Win32 Static Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "allegrogl - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "allegrogl - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "allegrogl - Win32 Static Release CRT" (based on "Win32 (x86) Static Library")
!MESSAGE "allegrogl - Win32 Static Release" (based on "Win32 (x86) Static Library")
!MESSAGE "allegrogl - Win32 Static Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "allegrogl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../lib/msvc/"
# PROP Intermediate_Dir "../../../obj/msvc/release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /Oy- /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /Gs /GF /GA /c
# ADD BASE RSC /l 0x41a /d "NDEBUG"
# ADD RSC /l 0x41a /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../../../lib/msvc/agl.lib"

!ELSEIF  "$(CFG)" == "allegrogl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../../lib/msvc"
# PROP Intermediate_Dir "../../../obj/msvc/debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "DEBUGMODE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x41a /d "_DEBUG"
# ADD RSC /l 0x41a /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../../../lib/msvc/agld.lib"

!ELSEIF  "$(CFG)" == "allegrogl - Win32 Static Release CRT"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Static Release CRT"
# PROP BASE Intermediate_Dir "Static Release CRT"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../lib/msvc"
# PROP Intermediate_Dir "../../../obj/msvc/release_s_crt"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MD /W3 /GX /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GX /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "ALLEGRO_STATICLINK" /YX /FD /c
# ADD BASE RSC /l 0x41a /d "NDEBUG"
# ADD RSC /l 0x41a /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"../../../lib/msvc/agl_s.lib"
# ADD LIB32 /nologo /out:"../../../lib/msvc/agl_s_crt.lib"

!ELSEIF  "$(CFG)" == "allegrogl - Win32 Static Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "allegrogl___Win32_Static_Release"
# PROP BASE Intermediate_Dir "allegrogl___Win32_Static_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../lib/msvc"
# PROP Intermediate_Dir "../../../obj/msvc/release_s"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W3 /GX /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "ALLEGRO_STATICLINK" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "ALLEGRO_STATICLINK" /YX /FD /c
# ADD BASE RSC /l 0x41a /d "NDEBUG"
# ADD RSC /l 0x41a /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"../../../lib/msvc/agl_s_crt.lib"
# ADD LIB32 /nologo /out:"../../../lib/msvc/agl_s.lib"

!ELSEIF  "$(CFG)" == "allegrogl - Win32 Static Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "allegrogl___Win32_Static_Debug"
# PROP BASE Intermediate_Dir "allegrogl___Win32_Static_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../../lib/msvc"
# PROP Intermediate_Dir "../../../obj/msvc/debug_s"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "DEBUGMODE" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "DEBUGMODE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x41a /d "_DEBUG"
# ADD RSC /l 0x41a /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"../../../lib/msvc/agld.lib"
# ADD LIB32 /nologo /out:"../../../lib/msvc/agld_s.lib"

!ENDIF 

# Begin Target

# Name "allegrogl - Win32 Release"
# Name "allegrogl - Win32 Debug"
# Name "allegrogl - Win32 Static Release CRT"
# Name "allegrogl - Win32 Static Release"
# Name "allegrogl - Win32 Static Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\src\aglf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\alleggl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\fontconv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\glext.c

!IF  "$(CFG)" == "allegrogl - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "allegrogl - Win32 Debug"

!ELSEIF  "$(CFG)" == "allegrogl - Win32 Static Release CRT"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "allegrogl - Win32 Static Release"

# ADD BASE CPP /O2
# ADD CPP /O2

!ELSEIF  "$(CFG)" == "allegrogl - Win32 Static Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\src\glvtable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\gui.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\math.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\scorer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\texture.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\videovtb.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\win.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\include\alleggl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allglint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\gl_ext.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\GLext\gl_ext_alias.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\GLext\gl_ext_api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\GLext\gl_ext_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\GLext\gl_ext_list.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\gl_header_detect.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\glvtable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\GLext\wgl_ext_alias.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\GLext\wgl_ext_api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\GLext\wgl_ext_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allegrogl\GLext\wgl_ext_list.h
# End Source File
# End Group
# End Target
# End Project
