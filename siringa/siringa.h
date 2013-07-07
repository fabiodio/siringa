<<<<<<< HEAD
/*
 * Siringa by Fab1o (fabiodio)
 * Dll injection tool
 * TODO: 
 * Multiple injection methods (CreateRemoteThread & WriteProcessMemory(OK), The CreateRemoteThread & LoadLibrary Technique, Manual Mapping, etc.);
 * Game profiles.
 */

=======
/****************************************************
 *	Siringa
 *	author:	Fab1o (fabiodio)
 *	info:	Dll injection tool
 *	notes:	It supports 4 injection modules:
 *			CreateRemoteThread, NtCreateThreadEx,
 *			SetWindowsHook, QueueUserAPC.
 *			Auto/manual-injection, multiple .dll
 *			injection.
 *	TODO:	Game profiles, start-up injection,
 *			start-up injection w/ command.
 ***************************************************/
>>>>>>> Update
#pragma once
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include "detours/detours.h"
#include "resource.h"

#define APP_NAME "Siringa"				// DON'T CHANGE PLEASE
#define APP_AUTHOR "Fab1o (fabiodio)"	// DON'T CHANGE PLEASE
#define MAX_DLLS 25
#define Popup( text ) MessageBox( GetForegroundWindow(), text, APP_NAME, MB_OK );

extern DWORD dwInjThread();
extern char szExe[MAX_PATH];
extern char szDll[MAX_DLLS][MAX_PATH];
extern int iMethod;
extern int bAuto;

/********************************************************************
 *	Thanks to securityxploded.com
 *	Struct and typedefinition for NtCreateThreadEx injection module
 *******************************************************************/
struct NtCreateThreadExBuffer
{
	ULONG Size;
	ULONG Unknown1;
	ULONG Unknown2;
	PULONG Unknown3;
	ULONG Unknown4;
	ULONG Unknown5;
	ULONG Unknown6;
	PULONG Unknown7;
	ULONG Unknown8;
};

typedef NTSTATUS (WINAPI *LPFUN_NtCreateThreadEx)(
	OUT PHANDLE hThread,
	IN ACCESS_MASK DesiredAccess,
	IN LPVOID ObjectAttributes,
	IN HANDLE ProcessHandle,
	IN LPTHREAD_START_ROUTINE lpStartAddress,
	IN LPVOID lpParameter,
	IN BOOL CreateSuspended,
	IN ULONG StackZeroBits,
	IN ULONG SizeOfStackCommit,
	IN ULONG SizeOfStackReserve,
	OUT LPVOID lpBytesBuffer
);

bool IsNullOrEmpty( const char* str );
DWORD GetProcessId( char *szExeName );
BOOL CreateRemoteThreadInjection( DWORD dwProcId, const char *szDllName );
<<<<<<< HEAD
=======
BOOL NtCreateThreadExInjection( DWORD dwProcId, const char *szDllName );
BOOL WindowsHookInjection( DWORD dwProcId, const char *szDllName );
BOOL APCInjection( DWORD dwProcId, const char *szDllName );
>>>>>>> Update
