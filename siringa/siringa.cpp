#include "siringa.h"

bool bIsProcessRunning( char *szExeName )
{
	if ( szExeName == NULL )
		return false;

	char szProcessName[MAX_PATH], szProcessName2[MAX_PATH];
    DWORD aProcesses[1024], cb, cProcesses;
	HANDLE hProcess = NULL;
	HMODULE hMod = NULL;
	UINT i = 0;

	memcpy( szProcessName2, szExeName, sizeof( szProcessName2 ) );

    if( !EnumProcesses( aProcesses, sizeof( aProcesses ), &cb ) )
        return FALSE;

    cProcesses = cb / sizeof( DWORD );

    for( i = 0; i < cProcesses; i++ )
	{
		hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, aProcesses[i] );

		if( hProcess )
		{
			if( EnumProcessModules( hProcess, &hMod, sizeof( hMod ), &cb ) )
			{
				GetModuleBaseName( hProcess, hMod, szProcessName, sizeof( szProcessName ) / sizeof( TCHAR ) );
			}

			_strlwr_s( szProcessName );
			_strlwr_s( szProcessName2 );
			
			if( strcmp( szProcessName, szProcessName2 ) == 0 )
			{
				return true;
			}
		}

		CloseHandle( hProcess );
	}

	return false;
}

DWORD GetProcessId( char *szExeName )
{ 
	DWORD dwRet = 0;
	DWORD dwCount = 0;

	HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

	if( hSnapshot != INVALID_HANDLE_VALUE )
	{
		PROCESSENTRY32 pe = { 0 };
		pe.dwSize = sizeof( PROCESSENTRY32 );

		BOOL bRet = Process32First( hSnapshot, &pe );

		while( bRet )
		{
			if( !_stricmp( pe.szExeFile, szExeName ) )
			{
				dwCount++;
				dwRet = pe.th32ProcessID;
			}
			bRet = Process32Next( hSnapshot, &pe );
		}

		if( dwCount > 1 )
			dwRet = 0xFFFFFFFF;

		CloseHandle( hSnapshot );
	}

	return dwRet;
}

BOOL CreateRemoteThreadInjection( DWORD dwProcId, const char *szDllName )
{
	HANDLE hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcId );
	LPVOID address = ( LPVOID )GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" );
	LPVOID memory = ( LPVOID )VirtualAllocEx( hProc, NULL, strlen( szDllName ), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE );

	if(!dwProcId)
		return false;

	WriteProcessMemory( hProc, memory,  szDllName, strlen( szDllName ), NULL );
	CreateRemoteThread( hProc, NULL, NULL, ( LPTHREAD_START_ROUTINE )address, memory, NULL, NULL );

	CloseHandle( hProc );

	return true;
}

DWORD dwInjThread()
{
	while(1)
	{
		if( bIsProcessRunning( szExe ) )
		{
			for( int i = 0; i < MAX_DLLS; i++ )
			{
				if( !strlen( szDll[i] ) )
					continue;

				CreateRemoteThreadInjection( GetProcessId( szExe ), szDll[i] );
			}

			Sleep( 500 );
		}
	}

	return 0;
}