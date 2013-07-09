/****************************************************
 *	Siringa
 *	version:	1.0
 *	author:		Fab1o (fabiodio)
 *	infos:		Dll injection tool
 *	notes:		It supports 5 injection methods:
 *				CreateRemoteThread, NtCreateThreadEx,
 *				RtlCreateUserThread, SetWindowsHook, 
 *				QueueUserAPC.
 *				Auto/manual-injection, multiple .dll
 *				injection.
 *	thanks:		undocumented.ntinternals.net - for the
 *				structs ofundocumented  native funcs;
 *				Sinner - application inspiration.
 *	TODO:		Game profiles, spawn-injection,
 *				spawn-injection w/ command.
 ***************************************************/
#pragma once
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "imagehlp.lib")
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <Psapi.h>
#include <imagehlp.h>
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
extern char szFuncName[MAX_PATH];
extern int iMethod;
extern int bAuto;

using namespace std; //xd

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

typedef struct _CLIENT_ID
{
	PVOID UniqueProcess;
	PVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

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

typedef NTSTATUS ( WINAPI *LPFUN_RtlCreateUserThread )(
	IN HANDLE ProcessHandle,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN BOOLEAN CreateSuspended,
	IN ULONG StackZeroBits,
	IN OUT PULONG StackReserved,
	IN OUT PULONG StackCommit,
	IN PVOID StartAddress,
	IN PVOID StartParameter,
	OUT PHANDLE ThreadHandle,
	OUT PCLIENT_ID ClientID
);

bool IsNullOrEmpty( const char* str );
bool bIsProcessRunning( char *szExeName );
DWORD GetProcessId( char *szExeName );
void GetDllFunctions( const char *szDllName, vector<string>& sList );
BOOL CreateRemoteThreadInjection( DWORD dwProcId, const char *szDllName );
BOOL NtCreateThreadExInjection( DWORD dwProcId, const char *szDllName );
BOOL WindowsHookInjection( DWORD dwProcId, const char *szDllName );
BOOL APCInjection( DWORD dwProcId, const char *szDllName );
BOOL RtlCreateUserThreadInjection( DWORD dwProcId, const char *szDllName );
