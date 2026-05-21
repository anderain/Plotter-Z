# Microsoft Developer Studio Project File - Name="PlotterZ" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE x86em) Application" 0x0b01
# TARGTYPE "Win32 (WCE MIPS) Application" 0x0a01
# TARGTYPE "Win32 (WCE SH) Application" 0x0901

CFG=PlotterZ - Win32 (WCE x86em) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PlotterZ.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PlotterZ.mak" CFG="PlotterZ - Win32 (WCE x86em) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PlotterZ - Win32 (WCE x86em) Release" (based on\
 "Win32 (WCE x86em) Application")
!MESSAGE "PlotterZ - Win32 (WCE x86em) Debug" (based on\
 "Win32 (WCE x86em) Application")
!MESSAGE "PlotterZ - Win32 (WCE MIPS) Release" (based on\
 "Win32 (WCE MIPS) Application")
!MESSAGE "PlotterZ - Win32 (WCE MIPS) Debug" (based on\
 "Win32 (WCE MIPS) Application")
!MESSAGE "PlotterZ - Win32 (WCE SH) Release" (based on\
 "Win32 (WCE SH) Application")
!MESSAGE "PlotterZ - Win32 (WCE SH) Debug" (based on\
 "Win32 (WCE SH) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP WCE_Configuration "H/PC Ver. 1.00"

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "x86emRel"
# PROP BASE Intermediate_Dir "x86emRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86emRel"
# PROP Intermediate_Dir "x86emRel"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /ML /W3 /O2 /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /YX /c
# ADD CPP /nologo /ML /W3 /O2 /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "NDEBUG"
# ADD RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /subsystem:windows /machine:I386 /windowsce:emulation
# ADD LINK32 commctrl.lib coredll.lib /nologo /subsystem:windows /machine:I386 /windowsce:emulation
EMPFILE=empfile.exe
# ADD BASE EMPFILE -COPY
# ADD EMPFILE -COPY

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86emDbg"
# PROP BASE Intermediate_Dir "x86emDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86emDbg"
# PROP Intermediate_Dir "x86emDbg"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Gm /Zi /Od /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /YX /c
# ADD CPP /nologo /MLd /W3 /Gm /Zi /Od /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "_DEBUG" /d "x86" /d "i486" /d "_x86_"
# ADD RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "_DEBUG" /d "x86" /d "i486" /d "_x86_"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /subsystem:windows /debug /machine:I386 /windowsce:emulation
# ADD LINK32 commctrl.lib coredll.lib /nologo /subsystem:windows /debug /machine:I386 /windowsce:emulation
EMPFILE=empfile.exe
# ADD BASE EMPFILE -COPY
# ADD EMPFILE -COPY

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WMIPSRel"
# PROP BASE Intermediate_Dir "WMIPSRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WMIPSRel"
# PROP Intermediate_Dir "WMIPSRel"
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /ML /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /QMRWCE /c
# ADD CPP /nologo /ML /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /QMRWCE /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSDbg"
# PROP BASE Intermediate_Dir "WMIPSDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WMIPSDbg"
# PROP Intermediate_Dir "WMIPSDbg"
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /QMRWCE /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /QMRWCE /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /debug /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /debug /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESHRel"
# PROP BASE Intermediate_Dir "WCESHRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCESHRel"
# PROP Intermediate_Dir "WCESHRel"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /ML /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /c
# ADD CPP /nologo /ML /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /machine:SH3 /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /machine:SH3 /subsystem:$(CESubsystem)
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESHDbg"
# PROP BASE Intermediate_Dir "WCESHDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCESHDbg"
# PROP Intermediate_Dir "WCESHDbg"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /debug /machine:SH3 /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /debug /machine:SH3 /subsystem:$(CESubsystem)
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ENDIF 

# Begin Target

# Name "PlotterZ - Win32 (WCE x86em) Release"
# Name "PlotterZ - Win32 (WCE x86em) Debug"
# Name "PlotterZ - Win32 (WCE MIPS) Release"
# Name "PlotterZ - Win32 (WCE MIPS) Debug"
# Name "PlotterZ - Win32 (WCE SH) Release"
# Name "PlotterZ - Win32 (WCE SH) Debug"
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\plotter-z-win32-native.c"

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\deps\pine89\pine-ini.h"\
	"..\..\..\deps\salvia89\salvia.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	"..\..\utils\hybird_6x8.h"\
	"..\..\utils\samples.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\PlotterZNative.rc

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

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

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\formula-z\parser.c"

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "EZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\evaluator-z\emitter.c"

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\evaluator-z\eval.c"

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

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

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\renderer-z\rz.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\renderer-z\transform.c"

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\constants.c

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\common\constants.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils.c

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\vlist.c

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

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

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_SALVI=\
	"..\..\..\deps\salvia89\salvia.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\deps\salvia89\salvia.h
# End Source File
# End Group
# Begin Group "Pine"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\deps\pine89\pine-ini.c"

!IF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE x86em) Debug"

DEP_CPP_PINE_=\
	"..\..\..\deps\pine89\pine-ini.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "PlotterZ - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\deps\pine89\pine-ini.h"
# End Source File
# End Group
# End Group
# End Target
# End Project
