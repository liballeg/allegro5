# Microsoft Developer Studio Project File - Name="excamera" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=excamera - Win32 Static Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "excamera.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "excamera.mak" CFG="excamera - Win32 Static Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "excamera - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "excamera - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "excamera - Win32 Static Release CRT" (based on "Win32 (x86) Application")
!MESSAGE "excamera - Win32 Release DLL" (based on "Win32 (x86) Application")
!MESSAGE "excamera - Win32 Static Release" (based on "Win32 (x86) Application")
!MESSAGE "excamera - Win32 Static Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "excamera - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../examp"
# PROP Intermediate_Dir "../../../obj/msvc/release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41a /d "NDEBUG"
# ADD RSC /l 0x41a /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 alleg.lib agl.lib opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /machine:I386 /libpath:"../../../lib/msvc"

!ELSEIF  "$(CFG)" == "excamera - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../../examp"
# PROP Intermediate_Dir "../../../obj/msvc/debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "DEBUGMODE" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41a /d "_DEBUG"
# ADD RSC /l 0x41a /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 alld.lib agld.lib opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"../../../lib/msvc"

!ELSEIF  "$(CFG)" == "excamera - Win32 Static Release CRT"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Static Release CRT"
# PROP BASE Intermediate_Dir "Static Release CRT"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../../examp"
# PROP Intermediate_Dir "../../../obj/msvc/release_s_crt"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "DEBUGMODE" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MT /W3 /GX /O2 /I "../../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "ALLEGRO_STATICLINK" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41a /d "_DEBUG"
# ADD RSC /l 0x41a /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 alld.lib agld_s.lib opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"../../../lib/msvc"
# ADD LINK32 alleg_s_crt.lib agl_s_crt.lib opengl32.lib glu32.lib user32.lib gdi32.lib dsound.lib dinput.lib ddraw.lib winmm.lib dxguid.lib ole32.lib kernel32.lib /nologo /subsystem:windows /incremental:no /machine:I386 /libpath:"../../../lib/msvc"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "excamera - Win32 Release DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release DLL"
# PROP BASE Intermediate_Dir "Release DLL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../examp"
# PROP Intermediate_Dir "../../../obj/msvc/release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MD /W3 /GX /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "ALLEGRO_GL_DYNAMIC" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41a /d "NDEBUG"
# ADD RSC /l 0x41a /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 alleg.lib agl_s.lib opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /machine:I386 /out:"../../../examp/excamera.exe" /libpath:"../../../lib/msvc"
# ADD LINK32 alleg.lib agl.lib opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /pdb:none /machine:I386 /libpath:"../../../lib/msvc/dll/"

!ELSEIF  "$(CFG)" == "excamera - Win32 Static Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "excamera___Win32_Static_Release"
# PROP BASE Intermediate_Dir "excamera___Win32_Static_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../../examp"
# PROP Intermediate_Dir "../../../obj/msvc/release_s"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W3 /GX /O2 /I "../../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "ALLEGRO_STATICLINK" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /I "../../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "ALLEGRO_STATICLINK" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41a /d "_DEBUG"
# ADD RSC /l 0x41a /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 alleg_s_crt.lib agl_s_crt.lib opengl32.lib glu32.lib user32.lib gdi32.lib dsound.lib dinput.lib ddraw.lib winmm.lib dxguid.lib ole32.lib kernel32.lib /nologo /subsystem:windows /incremental:no /machine:I386 /libpath:"../../../lib/msvc"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 alleg_s.lib agl_s.lib opengl32.lib glu32.lib user32.lib gdi32.lib dsound.lib dinput.lib ddraw.lib winmm.lib dxguid.lib ole32.lib kernel32.lib /nologo /subsystem:windows /incremental:no /machine:I386 /libpath:"../../../lib/msvc"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "excamera - Win32 Static Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "excamera___Win32_Static_Debug"
# PROP BASE Intermediate_Dir "excamera___Win32_Static_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../../examp"
# PROP Intermediate_Dir "../../../obj/msvc/debug_s"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "DEBUGMODE" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "DEBUGMODE" /D "ALLEGRO_STATICLINK" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41a /d "_DEBUG"
# ADD RSC /l 0x41a /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 alld.lib agld.lib opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"../../../lib/msvc"
# ADD LINK32 alld_s.lib agld_s.lib opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"../../../lib/msvc"

!ENDIF 

# Begin Target

# Name "excamera - Win32 Release"
# Name "excamera - Win32 Debug"
# Name "excamera - Win32 Static Release CRT"
# Name "excamera - Win32 Release DLL"
# Name "excamera - Win32 Static Release"
# Name "excamera - Win32 Static Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\examp\excamera.c
# End Source File
# End Group
# End Target
# End Project
