#include "siringa.h"

BOOL CALLBACK MainDlgProc( HWND, UINT, WPARAM, LPARAM );

HINSTANCE g_hInstance = NULL;
HANDLE hInjThread = NULL;

char szExe[ MAX_PATH ];
char szDll[ MAX_DLLS ][ MAX_PATH ];

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
		case IDC_ADDDLL:
			char filePath[ MAX_PATH ], fileDll[ MAX_PATH ];
			memset( filePath, 0, sizeof( filePath ) );
			memset( fileDll, 0, sizeof( fileDll ) );
			GetDllDialog( filePath );
			/* file name
			char *ptr = filePath;
			while( *ptr ) *ptr++;
			while( *ptr != '\\' ) *ptr--; *ptr++;
			strcpy_s( fileDll, ptr );
			*/
			SendMessage( GetDlgItem( hDlg, IDC_LISTDLL ), LB_ADDSTRING, NULL, ( LPARAM )filePath );
			break;
		case IDC_DELDLL:
			LRESULT selectedItm = SendMessage( GetDlgItem( hDlg, IDC_LISTDLL ), LB_GETCURSEL, NULL, NULL );
			SendMessage( GetDlgItem( hDlg, IDC_LISTDLL ), LB_DELETESTRING, selectedItm, NULL );
			break;
		}
		switch( LOWORD( wParam ) )
		{
		case IDC_LISTDLL:
			if( HIWORD( wParam ) == LBN_SELCHANGE )
			{
				int iBuffer[ MAX_DLLS ];
				memset( iBuffer, 0, MAX_DLLS );
				memset( szDll, 0, sizeof( szDll ) );

				HWND hListBox = GetDlgItem( hDlg, IDC_LISTDLL );
				LRESULT itemsInBuffer = SendMessage( hListBox, LB_GETSELITEMS, MAX_DLLS, ( LPARAM )iBuffer );
			
				for( int i = 0; i < ( int )itemsInBuffer; i++ )
				{
					SendMessage( hListBox, LB_GETTEXT, iBuffer[ i ], ( LPARAM )szDll[ i ] );
				}
			}
			break;
		}
		return true;

	case WM_CLOSE:
		EndDialog( hDlg, 0 );
		return true;
	}
	return false;
}
