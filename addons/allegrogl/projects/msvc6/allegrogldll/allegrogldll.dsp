# Microsoft Developer Studio Project File - Name="allegrogl dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=allegrogl dll - Win32 Release DLL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "allegrogl dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "allegrogl dll.mak" CFG="allegrogl dll - Win32 Release DLL"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "allegrogl dll - Win32 Release DLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release DLL"
# PROP BASE Intermediate_Dir "Release DLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../lib/msvc/dll"
# PROP Intermediate_Dir "../../../obj/msvc/dll_release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ALLEGRO_GL_DYNAMIC" /D "ALLEGRO_GL_SRC_BUILD" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ALLEGRO_GL_DYNAMIC" /D "ALLEGRO_GL_SRC_BUILD" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41a /d "NDEBUG"
# ADD RSC /l 0x41a /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 alleg.lib opengl32.lib glu32.lib user32.lib gdi32.lib dsound.lib dinput.lib ddraw.lib winmm.lib dxguid.lib ole32.lib kernel32.lib /nologo /version:0.43 /dll /machine:I386 /out:"../../../lib/msvc/agl.dll"
# Begin Target

# Name "allegrogl dll - Win32 Release DLL"
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
