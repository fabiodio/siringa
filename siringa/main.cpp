#include "siringa.h"

BOOL CALLBACK MainDlgProc( HWND, UINT, WPARAM, LPARAM );

HINSTANCE g_hInstance = NULL;

HANDLE hInjThread = NULL;

char szExe[MAX_PATH];
char szDll[MAX_DLLS][MAX_PATH];
int iMethod;
int bAuto;

char *trim( char *str )
{
	char *end;

	while( isspace( *str ) ) str++;

	if(*str == 0)
	return str;

	end = str + strlen( str ) - 1;
	while( end > str && isspace( *end ) ) end--;

	*( end + 1 ) = 0;

	return str;
}

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
	HANDLE handle = FindFirstFile( fileName, &findData );
	return ( handle != INVALID_HANDLE_VALUE );
}

int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	memset( szExe, 0, sizeof( szExe ) );
	memset( szDll, 0, sizeof( szDll ) );

	g_hInstance = hInstance;

	if( ( hInjThread = CreateThread( NULL, 0, ( LPTHREAD_START_ROUTINE )dwInjThread, 0, 0, 0 ) ) == NULL )
	{
		MessageBox( GetForegroundWindow(), "Failed to create injection thread!", APP_NAME, MB_OK | MB_ICONERROR );
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
		iMethod = 0;
		bAuto = 1;

		SetWindowTextA( hDlg, APP_NAME );
		SetDlgItemText( hDlg, IDC_EXE, "example.exe" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"CreateRemoteThread method" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"NtCreateThreadEx method" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"Windows Hooks method" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"APC method" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_SETCURSEL, 0, NULL );
		SendMessage( GetDlgItem( hDlg, IDC_AUTO ), BM_SETCHECK, bAuto, NULL );

		return true;
	case WM_COMMAND:
		switch( wParam )
		{
		case IDC_ADDEXE:
			{
				char fileExe[MAX_PATH];
				memset( fileExe, 0, sizeof( fileExe ) );
				GetDlgItemText( hDlg, IDC_EXE, fileExe, MAX_PATH );
				if( IsNullOrEmpty( trim( fileExe ) ) )
					break;
				SendMessage( GetDlgItem( hDlg, IDC_LISTEXE ), LB_ADDSTRING, NULL, ( LPARAM )trim( fileExe ) );
				SetDlgItemText( hDlg, IDC_EXE, NULL );
				memset( fileExe, 0, sizeof( fileExe ) );
			}
			break;
		case IDC_DELEXE:
			{
				LRESULT iExe = SendMessage( GetDlgItem( hDlg, IDC_LISTEXE ), LB_GETCURSEL, NULL, NULL );
				SendMessage( GetDlgItem( hDlg, IDC_LISTEXE ), LB_DELETESTRING, iExe, NULL );
				memset( szExe, 0, sizeof( szExe ) );
			}
			break;
		case IDC_ADDDLL:
			{
				char filePath[MAX_PATH];
				memset( filePath, 0, sizeof( filePath ) );
				GetDllDialog( filePath );
				/* file name
				char *ptr = filePath;
				while( *ptr ) *ptr++;
				while( *ptr != '\\' ) *ptr--; *ptr++;
				strcpy_s( fileDll, ptr );
				*/
				if( IsNullOrEmpty( filePath ) )
					break;
				SendMessage( GetDlgItem( hDlg, IDC_LISTDLL ), LB_ADDSTRING, NULL, ( LPARAM )filePath );
				memset( filePath, 0, sizeof( filePath ) );
			}
			break;
		case IDC_DELDLL:
			{
				LRESULT iDll = SendMessage( GetDlgItem( hDlg, IDC_LISTDLL ), LB_GETCURSEL, NULL, NULL );
				SendMessage( GetDlgItem( hDlg, IDC_LISTDLL ), LB_DELETESTRING, iDll, NULL );
			}
			break;
		}
		switch( LOWORD( wParam ) )
		{
		case IDC_LISTDLL:
			{
				if( HIWORD( wParam ) == LBN_SELCHANGE )
				{
					int iBuffer[MAX_DLLS];
					memset( iBuffer, 0, MAX_DLLS );
					memset( szDll, 0, sizeof( szDll ) );

					HWND hListBox = GetDlgItem( hDlg, IDC_LISTDLL );
					LRESULT itemsInBuffer = SendMessage( hListBox, LB_GETSELITEMS, MAX_DLLS, ( LPARAM )iBuffer );
			
					for( int i = 0; i < ( int )itemsInBuffer; i++ )
					{
						SendMessage( hListBox, LB_GETTEXT, iBuffer[i], ( LPARAM )szDll[i] );
					}
				}
			}
			break;
		case IDC_LISTEXE:
			{
				if( HIWORD( wParam ) == LBN_SELCHANGE )
				{
					HWND hListBox = GetDlgItem( hDlg, IDC_LISTEXE );
					int iItem = SendMessage( hListBox, LB_GETCURSEL, 0, 0 );

					SendMessage( hListBox, LB_GETTEXT, iItem, ( LPARAM )szExe );
				}
			}
			break;
		case IDC_METHOD:
			{
				if( HIWORD( wParam ) == CBN_SELCHANGE )
				{
					iMethod = SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_GETCURSEL, 0, 0 );
				}
			}
		case IDC_AUTO:
			{
				if( HIWORD( wParam ) == BN_CLICKED )
				{
					bAuto = SendMessage( GetDlgItem( hDlg, IDC_AUTO ), BM_GETCHECK, 0, 0 );
				}
			}
		}
		return true;

	case WM_CLOSE:
		EndDialog( hDlg, 0 );
		return true;
	}
	return false;
}
