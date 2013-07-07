#include "siringa.h"

bool IsNullOrEmpty( const char* str )
{
	return (str == 0) || (*str == '\0');
}

bool bIsProcessRunning( char *szExeName )
{
	if( IsNullOrEmpty( szExeName ) )
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
/*	old
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
*/
BOOL CreateRemoteThreadInjection( DWORD dwProcId, const char *szDllName )
{
	if(!dwProcId)
		return false;

	HANDLE hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcId );
	LPVOID memory = VirtualAllocEx( hProc, NULL, strlen( szDllName ), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
	WriteProcessMemory( hProc, memory, szDllName, strlen( szDllName ), NULL );
	
	HANDLE hThread = CreateRemoteThread( hProc, NULL, 0, ( LPTHREAD_START_ROUTINE )GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" ), memory, 0, NULL );
	WaitForSingleObject( hThread, INFINITE );
	
	CloseHandle( hThread );
	VirtualFreeEx( hProc, memory, sizeof( szDllName ), MEM_RELEASE );

	return true;
}

BOOL WindowsHookInjection( DWORD dwProcId, const char *szDllName )
{
	if(!dwProcId)
		return false;

	HMODULE hDll = LoadLibrary( szDllName );
	HOOKPROC procAddress = ( HOOKPROC )GetProcAddress( hDll, "FuncName" );

	SetWindowsHookEx( WH_CBT, procAddress, hDll, dwProcId );

	return true;
}

DWORD __allocStub(DWORD m_dwPID, const char *m_pwszStubInfo) { // Thanks to betamonkey
    if ( m_dwPID == NULL || m_pwszStubInfo == NULL )
        return NULL;

    // Locals
    LPVOID m_dwStubAddr  = NULL;
    DWORD  m_dwStubLen   = lstrlen(m_pwszStubInfo) * 2; // Compensate for Unicode
    DWORD  m_dwNumBytesW = NULL;
    DWORD  m_dwReturn    = NULL;
    HANDLE m_hProcess    = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_dwPID);

    if ( m_hProcess ) {
        // Allocate memory for our stub + other stuff (dll path string, for instance)
        m_dwStubAddr = VirtualAllocEx(m_hProcess, NULL, m_dwStubLen, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        if ( m_dwStubAddr ) {
            // Write stub to remote memory
            if ( WriteProcessMemory(m_hProcess, m_dwStubAddr, (LPCVOID)m_pwszStubInfo, m_dwStubLen, &m_dwNumBytesW) == TRUE )
                m_dwReturn = (DWORD) m_dwStubAddr;
            else
                // Free it since we couldn't write our stub.. No point in having a NULL block of memory sitting around
                VirtualFreeEx(m_hProcess, m_dwStubAddr, m_dwStubLen, MEM_COMMIT | MEM_RESERVE);
        }

        // Cleanup
        CloseHandle(m_hProcess);
    }

    // Return the location of the stub in remote process' virtual memory
    return m_dwReturn;
}

DWORD __sprayThreads(DWORD m_dwPID, DWORD m_dwLoadLibAddr, DWORD m_dwPathAddr) { // Thanks to betamonkey
    if ( m_dwPID == NULL || m_dwLoadLibAddr == NULL || m_dwPathAddr == NULL )
        return NULL;

    THREADENTRY32  m_stPEThread;
    HANDLE    m_hSnap = 0;

    // Grab a list of threads for the process
    if ( (m_hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)) == INVALID_HANDLE_VALUE )
        return 0;

    memset(&m_stPEThread, 0, sizeof(m_stPEThread));
    m_stPEThread.dwSize = sizeof(THREADENTRY32);

    // Grab first thread in list, execute if no thread found
    if ( Thread32First(m_hSnap, &m_stPEThread) == FALSE ) {
        CloseHandle(m_hSnap);

        return NULL;
    }

    do {
        if ( m_stPEThread.th32OwnerProcessID == m_dwPID ) {
            HANDLE m_hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, m_stPEThread.th32ThreadID);

            if ( m_hThread ) {
                QueueUserAPC((PAPCFUNC)m_dwLoadLibAddr, m_hThread, (ULONG_PTR)m_dwPathAddr);
                CloseHandle(m_hThread);
            }
        }
    } while ( Thread32Next(m_hSnap, &m_stPEThread) );

    //  Don't forget to clean up the snapshot object.
    CloseHandle(m_hSnap);

    return 0;
}

UINT WINAPI MyThread(){ // Thanks to betamonkey
    HANDLE m_hEvent = CreateEvent(NULL, FALSE, FALSE, "APC");

    while(1)
        WaitForSingleObjectEx(m_hEvent, INFINITE, TRUE);

    CloseHandle( m_hEvent );

    return 0;
}

BOOL APCInjection( DWORD dwProcId, const char *szDllName ) // Thanks to betamonkey
{
	DWORD m_dwThreadId = NULL;
	HANDLE m_hMyThreadHandle = NULL;
	DWORD m_dwLoadLibraryW = ( DWORD )GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" );
	DWORD m_dwPathAddr = __allocStub( dwProcId, szDllName );

	if( m_dwPathAddr )
	{
		if ( ( m_hMyThreadHandle = CreateThread(NULL, NULL, ( LPTHREAD_START_ROUTINE )MyThread, NULL, NULL, &m_dwThreadId ) ) != NULL )
		{
			__sprayThreads( dwProcId, m_dwLoadLibraryW, ( ULONG_PTR )m_dwPathAddr );
        }
		else
			Popup( "Error!" );
	}
	else
		Popup( "Error!" );

	return true;
}

HANDLE bCreateRemoteThread( HANDLE hHandle, LPVOID loadLibAddr, LPVOID dllPathAddr )
{
	HANDLE hThread = NULL;

	LPVOID ntCreateThreadExAddr = NULL;
	NtCreateThreadExBuffer ntbuffer;
	DWORD temp1 = 0;
	DWORD temp2 = 0;

	ntCreateThreadExAddr = GetProcAddress( GetModuleHandle( TEXT( "ntdll.dll" ) ), "NtCreateThreadEx" );

	if( ntCreateThreadExAddr )
	{
		ntbuffer.Size = sizeof( struct NtCreateThreadExBuffer );
		ntbuffer.Unknown1 = 0x10003;
		ntbuffer.Unknown2 = 0x8;
		ntbuffer.Unknown3 = &temp2;
		ntbuffer.Unknown4 = 0;
		ntbuffer.Unknown5 = 0x10004;
		ntbuffer.Unknown6 = 4;
		ntbuffer.Unknown7 = &temp1;
		ntbuffer.Unknown8 = 0;

		LPFUN_NtCreateThreadEx funNtCreateThreadEx = ( LPFUN_NtCreateThreadEx )ntCreateThreadExAddr;
		NTSTATUS status = funNtCreateThreadEx( &hThread, 0x1FFFFF, NULL, hHandle, ( LPTHREAD_START_ROUTINE )loadLibAddr, dllPathAddr, FALSE, NULL, NULL, NULL, &ntbuffer );
  
		if ( !hThread )
		{
			Popup( "NtCreateThreadEx Failed [%d][%08x]", GetLastError(), status );
			return 0;
		} 
		else
			return hThread;
	}
	else
		Popup( "Could not find NtCreateThreadEx" );

	return 0;
}

BOOL NtCreateThreadExInjection( DWORD dwProcId, const char *szDllName )
{
	if(!dwProcId)
		return false;

	HANDLE hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcId );
	LPVOID memory = VirtualAllocEx( hProc, NULL, strlen( szDllName ), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
	WriteProcessMemory( hProc, memory, szDllName, strlen( szDllName ), NULL );
	
	HANDLE hThread = bCreateRemoteThread( hProc, ( LPTHREAD_START_ROUTINE )GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" ), memory );
	WaitForSingleObject( hThread, INFINITE );
	
	CloseHandle( hThread );

	return true;
}

HANDLE bRtlCreateUserThread( HANDLE hHandle, LPVOID loadLibAddr, LPVOID dllPathAddr )
{
	HANDLE hThread = NULL;
	LPVOID rtlCreateUserAddr = NULL;

	CLIENT_ID cId;

	rtlCreateUserAddr = GetProcAddress( GetModuleHandle( "ntdll.dll" ), "RtlCreateUserThread" );

	if( rtlCreateUserAddr )
	{
		LPFUN_RtlCreateUserThread funRtlCreateUserThread = ( LPFUN_RtlCreateUserThread )rtlCreateUserAddr;

		NTSTATUS status = funRtlCreateUserThread( hHandle, NULL, false, 0, 0, 0, loadLibAddr, dllPathAddr, &hThread, &cId );

		if( hThread == NULL )
		{
			Popup( "NtCreateThreadEx Failed [%d][%08x]" );
			return 0;
		}
		else
			return hThread;
	}
	else
		Popup( "Could not find RtlCreateUserThread" );
	
	return 0;
}

BOOL RtlCreateUserThreadInjection( DWORD dwProcId, const char *szDllName )
{
	if(!dwProcId)
		return false;

	HANDLE hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcId );
	LPVOID memory = VirtualAllocEx( hProc, NULL, strlen( szDllName ), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
	WriteProcessMemory( hProc, memory, szDllName, strlen( szDllName ), NULL );
	
	HANDLE hThread = bRtlCreateUserThread( hProc, ( LPTHREAD_START_ROUTINE )GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" ), memory );
	WaitForSingleObject( hThread, INFINITE );

	CloseHandle( hThread );

	return true;
}

DWORD dwInjThread()
{
	while(1)
	{
		if( bAuto && bIsProcessRunning( szExe ) && !IsNullOrEmpty( szDll[0] ) )
		{
			for( int i = 0; i < MAX_DLLS; i++ )
			{
				if( i == ( MAX_DLLS - 1 ) )
				{
					Sleep( 500 );
					exit(0);
				}

				if( !strlen( szDll[i] ) )
					continue;

				switch( iMethod )
				{
				case 0:
					CreateRemoteThreadInjection( GetProcessId( szExe ), szDll[i] );
					break;
				case 1:
					NtCreateThreadExInjection( GetProcessId( szExe ), szDll[i] );
					break;
				case 2:
					RtlCreateUserThreadInjection( GetProcessId( szExe ), szDll[i] );
					break;
				case 3:
					WindowsHookInjection( GetProcessId( szExe ), szDll[i] );
					break;
				case 4:
					APCInjection( GetProcessId( szExe ), szDll[i] );
					break;
				}
			}

			Sleep( 1000 );
		}
	}

	return 0;
}