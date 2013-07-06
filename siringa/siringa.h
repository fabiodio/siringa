#pragma once
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include "detours/detours.h"
#include "resource.h"

#define APP_NAME "Siringa"
#define APP_AUTHOR "Fab1o"
#define Popup( text ) MessageBox( GetForegroundWindow(), text, APP_NAME, MB_OK );

extern DWORD dwInjThread();
extern char szExe[ MAX_PATH ];
extern char szDll[ MAX_PATH ];

DWORD GetProcessId( char *szExeName );
BOOL CreateRemoteThreadInjection( DWORD dwProcId, const char *szDllName );