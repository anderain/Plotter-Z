# Microsoft Developer Studio Project File - Name="PlotterZClassic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE x86) Application" 0x8301
# TARGTYPE "Win32 (WCE ARM) Application" 0x8501
# TARGTYPE "Win32 (WCE PPC) Application" 0x8401
# TARGTYPE "Win32 (WCE x86em) Application" 0x7f01
# TARGTYPE "Win32 (WCE SH3) Application" 0x8101
# TARGTYPE "Win32 (WCE SH4) Application" 0x8601
# TARGTYPE "Win32 (WCE MIPS) Application" 0x8201
# TARGTYPE "Win32 (WCE MIPSFP) Application" 0x8701

CFG=PlotterZClassic - Win32 (WCE MIPS) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PlotterZClassic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PlotterZClassic.mak" CFG="PlotterZClassic - Win32 (WCE MIPS) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PlotterZClassic - Win32 (WCE MIPS) Release" (based on "Win32 (WCE MIPS) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE MIPS) Debug" (based on "Win32 (WCE MIPS) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE SH4) Release" (based on "Win32 (WCE SH4) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE SH4) Debug" (based on "Win32 (WCE SH4) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE SH3) Release" (based on "Win32 (WCE SH3) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE SH3) Debug" (based on "Win32 (WCE SH3) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE ARM) Release" (based on "Win32 (WCE ARM) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE ARM) Debug" (based on "Win32 (WCE ARM) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE MIPSFP) Release" (based on "Win32 (WCE MIPSFP) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE MIPSFP) Debug" (based on "Win32 (WCE MIPSFP) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE x86) Release" (based on "Win32 (WCE x86) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE x86) Debug" (based on "Win32 (WCE x86) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE x86em) Release" (based on "Win32 (WCE x86em) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE x86em) Debug" (based on "Win32 (WCE x86em) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE PPC) Release" (based on "Win32 (WCE PPC) Application")
!MESSAGE "PlotterZClassic - Win32 (WCE PPC) Debug" (based on "Win32 (WCE PPC) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "H/PC Ver. 2.00"
# PROP WCE_FormatVersion "6.0"

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

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
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /M$(CECrt) /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /QMRWCE /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x804 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSDbg"
# PROP BASE Intermediate_Dir "WMIPSDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WMIPSDbg"
# PROP Intermediate_Dir "WMIPSDbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /QMRWCE /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /debug /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /debug /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESH4Rel"
# PROP BASE Intermediate_Dir "WCESH4Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCESH4Rel"
# PROP Intermediate_Dir "WCESH4Rel"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /Qsh4 /MC /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /Qsh4 /MC /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "SHx" /d "SH4" /d "_SH4_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x804 /r /d "SHx" /d "SH4" /d "_SH4_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /machine:SH4 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /machine:SH4 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESH4Dbg"
# PROP BASE Intermediate_Dir "WCESH4Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCESH4Dbg"
# PROP Intermediate_Dir "WCESH4Dbg"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /Qsh4 /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /Qsh4 /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "SHx" /d "SH4" /d "_SH4_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x804 /r /d "SHx" /d "SH4" /d "_SH4_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /debug /machine:SH4 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /debug /machine:SH4 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESH3Rel"
# PROP BASE Intermediate_Dir "WCESH3Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCESH3Rel"
# PROP Intermediate_Dir "WCESH3Rel"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrt) /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x804 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /machine:SH3 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /machine:SH3 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESH3Dbg"
# PROP BASE Intermediate_Dir "WCESH3Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCESH3Dbg"
# PROP Intermediate_Dir "WCESH3Dbg"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x804 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /debug /machine:SH3 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /debug /machine:SH3 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCEARMRel"
# PROP BASE Intermediate_Dir "WCEARMRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCEARMRel"
# PROP Intermediate_Dir "WCEARMRel"
# PROP Target_Dir ""
CPP=clarm.exe
# ADD BASE CPP /nologo /MC /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "ARM" /D "_ARM_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MC /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "ARM" /D "_ARM_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "ARM" /d "_ARM_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x804 /r /d "ARM" /d "_ARM_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 coredll.lib commctrl.lib /nologo /machine:ARM /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 coredll.lib commctrl.lib /nologo /machine:ARM /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCEARMDbg"
# PROP BASE Intermediate_Dir "WCEARMDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCEARMDbg"
# PROP Intermediate_Dir "WCEARMDbg"
# PROP Target_Dir ""
CPP=clarm.exe
# ADD BASE CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "ARM" /D "_ARM_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "ARM" /D "_ARM_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "ARM" /d "_ARM_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x804 /r /d "ARM" /d "_ARM_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 coredll.lib commctrl.lib /nologo /debug /machine:ARM /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 coredll.lib commctrl.lib /nologo /debug /machine:ARM /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WMIPSFPRel"
# PROP BASE Intermediate_Dir "WMIPSFPRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WMIPSFPRel"
# PROP Intermediate_Dir "WMIPSFPRel"
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /QMFWCE /MC /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /QMFWCE /MC /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x804 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSFPDbg"
# PROP BASE Intermediate_Dir "WMIPSFPDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WMIPSFPDbg"
# PROP Intermediate_Dir "WMIPSFPDbg"
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /QMFWCE /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /QMFWCE /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x804 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /debug /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /debug /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCEX86Rel"
# PROP BASE Intermediate_Dir "WCEX86Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCEX86Rel"
# PROP Intermediate_Dir "WCEX86Rel"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /ML /W3 /O2 /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "NDEBUG" /D "i_386_" /D "_MBCS" /Yu"stdafx.h" /Gs8192 /GF /c
# ADD CPP /nologo /ML /W3 /O2 /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "NDEBUG" /D "i_386_" /D "_MBCS" /Gs8192 /GF /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x804 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /machine:IX86 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /machine:IX86 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCEX86Dbg"
# PROP BASE Intermediate_Dir "WCEX86Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCEX86Dbg"
# PROP Intermediate_Dir "WCEX86Dbg"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "DEBUG" /D "i_386_" /D "_MBCS" /Yu"stdafx.h" /Gs8192 /GF /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "DEBUG" /D "i_386_" /D "_MBCS" /Gs8192 /GF /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x804 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /debug /machine:IX86 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /debug /machine:IX86 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

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
# ADD BASE CPP /nologo /ML /W3 /O2 /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /ML /W3 /O2 /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "NDEBUG"
# ADD RSC /l 0x804 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /machine:I386 /nodefaultlib:"$(CENoDefaultLib)" /windowsce:emulation
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /machine:I386 /nodefaultlib:"$(CENoDefaultLib)" /windowsce:emulation

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86emDbg"
# PROP BASE Intermediate_Dir "x86emDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86emDbg"
# PROP Intermediate_Dir "x86emDbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Gm /Zi /Od /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MLd /W3 /Gm /Zi /Od /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "_DEBUG" /d "x86" /d "i486" /d "_x86_"
# ADD RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "_DEBUG" /d "x86" /d "i486" /d "_x86_"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /debug /machine:I386 /nodefaultlib:"$(CENoDefaultLib)" /windowsce:emulation
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /debug /machine:I386 /nodefaultlib:"$(CENoDefaultLib)" /windowsce:emulation

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCEPPCRel"
# PROP BASE Intermediate_Dir "WCEPPCRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCEPPCRel"
# PROP Intermediate_Dir "WCEPPCRel"
# PROP Target_Dir ""
CPP=clppc.exe
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "PPC" /D "_PPC_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrt) /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "PPC" /D "_PPC_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "ppc" /d "_ppc_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x804 /r /d "ppc" /d "_ppc_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /machine:PPC /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /machine:PPC /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCEPPCDbg"
# PROP BASE Intermediate_Dir "WCEPPCDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCEPPCDbg"
# PROP Intermediate_Dir "WCEPPCDbg"
# PROP Target_Dir ""
CPP=clppc.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "PPC" /D "_PPC_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "PPC" /D "_PPC_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /r /d "ppc" /d "_ppc_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x804 /r /d "ppc" /d "_ppc_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /debug /machine:PPC /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /debug /machine:PPC /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "PlotterZClassic - Win32 (WCE MIPS) Release"
# Name "PlotterZClassic - Win32 (WCE MIPS) Debug"
# Name "PlotterZClassic - Win32 (WCE SH4) Release"
# Name "PlotterZClassic - Win32 (WCE SH4) Debug"
# Name "PlotterZClassic - Win32 (WCE SH3) Release"
# Name "PlotterZClassic - Win32 (WCE SH3) Debug"
# Name "PlotterZClassic - Win32 (WCE ARM) Release"
# Name "PlotterZClassic - Win32 (WCE ARM) Debug"
# Name "PlotterZClassic - Win32 (WCE MIPSFP) Release"
# Name "PlotterZClassic - Win32 (WCE MIPSFP) Debug"
# Name "PlotterZClassic - Win32 (WCE x86) Release"
# Name "PlotterZClassic - Win32 (WCE x86) Debug"
# Name "PlotterZClassic - Win32 (WCE x86em) Release"
# Name "PlotterZClassic - Win32 (WCE x86em) Debug"
# Name "PlotterZClassic - Win32 (WCE PPC) Release"
# Name "PlotterZClassic - Win32 (WCE PPC) Debug"
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\plotter-z-win32-classic.c"

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_PLOTT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\utils\hybird_6x8.h"\
	

!ENDIF 

# End Source File
# End Group
# Begin Group "PlotterZ"

# PROP Default_Filter ""
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\PlotterZClassic.rc

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# End Group
# Begin Group "EZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\evaluator-z\emitter.c"

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_EMITT=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\evaluator-z\eval.c"

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_EVAL_=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\evaluator-z\ez.h"\
	"..\..\..\formula-z\fz.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\evaluator-z\ez.h"
# End Source File
# End Group
# Begin Group "FZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\formula-z\fz.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\formula-z\lexer.c"

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_LEXER=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\formula-z\parser.c"

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_PARSE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	

!ENDIF 

# End Source File
# End Group
# Begin Group "RZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\renderer-z\ascii_extended_mapping.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\renderer-z\render.c"

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_RENDE=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\renderer-z\rz.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\renderer-z\transform.c"

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_TRANS=\
	"..\..\..\common\utils.h"\
	"..\..\..\common\vlist.h"\
	"..\..\..\formula-z\fz.h"\
	"..\..\..\renderer-z\ascii_extended_mapping.h"\
	"..\..\..\renderer-z\rz.h"\
	

!ENDIF 

# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\constants.c

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_CONST=\
	"..\..\..\common\constants.h"\
	"..\..\..\common\utils.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\common\constants.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils.c

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_UTILS=\
	"..\..\..\common\utils.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\vlist.c

!IF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Release"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPS) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Release"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH4) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Release"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE SH3) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Release"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE ARM) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Release"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE MIPSFP) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Release"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Release"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE x86em) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Release"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ELSEIF  "$(CFG)" == "PlotterZClassic - Win32 (WCE PPC) Debug"

DEP_CPP_VLIST=\
	"..\..\..\common\vlist.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\common\vlist.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\PlotterZClassic.ico
# End Source File
# End Target
# End Project
