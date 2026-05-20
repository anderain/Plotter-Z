# Microsoft Developer Studio Project File - Name="PlotterZ" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=PlotterZ - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PlotterZ.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PlotterZ.mak" CFG="PlotterZ - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PlotterZ - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "PlotterZ - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
# PROP WCE_FormatVersion "6.0"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PlotterZ - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "PlotterZ - Win32 Release"
# Name "PlotterZ - Win32 Debug"
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\plotter-z-win32-native.c"
# End Source File
# Begin Source File

SOURCE=..\PlotterZNative.rc
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# End Group
# Begin Group "FZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\formula-z\fz.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\formula-z\lexer.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\formula-z\parser.c"
# End Source File
# End Group
# Begin Group "EZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\evaluator-z\emitter.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\evaluator-z\eval.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\evaluator-z\ez.h"
# End Source File
# End Group
# Begin Group "RZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\renderer-z\ascii_extended_mapping.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\renderer-z\render.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\renderer-z\rz.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\renderer-z\transform.c"
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\constants.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\constants.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\vlist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\vlist.h
# End Source File
# End Group
# Begin Group "Deps"

# PROP Default_Filter ""
# Begin Group "Salvia"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\deps\salvia89\salvia.c
# End Source File
# Begin Source File

SOURCE=..\..\..\deps\salvia89\salvia.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\PlotterZNative.ICO
# End Source File
# End Target
# End Project
