#include "siringa.h"

BOOL CALLBACK MainDlgProc( HWND, UINT, WPARAM, LPARAM );

HINSTANCE g_hInstance = NULL;
HANDLE hInjThread = NULL;

char szExe[ MAX_PATH ];
char szDll[ MAX_PATH ];

bool GetDllDialog( char * szDll )
{
	OPENFILENAME ofn;
	memset( &ofn, 0, sizeof( ofn ) );

	ofn.lStructSize = sizeof( ofn );
	ofn.hInstance = g_hInstance;
	ofn.lpstrFile = szDll;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = "Dynamic-link libraries\0*.Dll\0\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	return ( GetOpenFileName( &ofn ) !=0 );
}

bool bFileExists( const char *fileName )
{
	WIN32_FIND_DATA findData;
	HANDLE handle = FindFirstFile(fileName, &findData );
	return ( handle != INVALID_HANDLE_VALUE );
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	bAutoInject = false;

	memset(szExe, 0, sizeof(szExe));
	memset(szDll, 0, sizeof(szDll));

	g_hInstance = hInstance;

	if( ( hInjThread = CreateThread( NULL, 0, ( LPTHREAD_START_ROUTINE )dwInjThread, 0, 0, 0 ) ) == NULL )
	{
		MessageBox( GetForegroundWindow(), "Failed to create injection thread! (aborted)!", APP_NAME, MB_OK|MB_ICONERROR );
		return false;
	}

	DialogBoxA( NULL, MAKEINTRESOURCEA( IDD_SIRINGA ), NULL, MainDlgProc );

	return 0;
}

BOOL CALLBACK MainDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_INITDIALOG:
		SetWindowTextA( hDlg, APP_NAME );
		return true;

	case WM_COMMAND:
		switch( wParam )
		{
		case IDC_BROWSE:
			GetDllDialog( szDll );
			SetDlgItemTextA( hDlg, IDC_DLL, szDll );
			break;
		case IDC_BUTTON1:
			GetDlgItemText(hDlg, IDC_EXE, szExe, MAX_PATH );
			GetDlgItemText(hDlg, IDC_DLL, szDll, MAX_PATH );
			CreateRemoteThreadInjection( GetProcessId( szExe ), szDll);
			break;
		}
		return true;

	case WM_CLOSE:
		EndDialog( hDlg, 0 );
		return true;
	}
	return false;
}
