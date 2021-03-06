#include "siringa.h"

using namespace std;

BOOL CALLBACK MainDlgProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK FuncDlgProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK AboutDlgProc( HWND, UINT, WPARAM, LPARAM );

HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;

HANDLE hInjThread = NULL;

char szExe[MAX_PATH];
char szDll[MAX_DLLS][MAX_PATH];
char szFuncName[MAX_DLLS][MAX_PATH];
int iMethod;
int bAuto;
int bQuit;

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
	g_hInstance = hInstance;
	memset( szExe, 0, sizeof( szExe ) );
	memset( szDll, 0, sizeof( szDll ) );

	if( ( hInjThread = CreateThread( NULL, 0, ( LPTHREAD_START_ROUTINE )dwInjThread, 0, 0, 0 ) ) == NULL )
	{
		MessageBox( GetForegroundWindow(), "Failed to create injection thread!", APP_NAME, MB_OK | MB_ICONERROR );
		return false;
	}

	DialogBox( NULL, MAKEINTRESOURCEA( IDD_SIRINGA ), NULL, MainDlgProc );

	return 0;
}

BOOL CALLBACK MainDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	g_hWnd = hDlg;

	switch( message )
	{
	case WM_INITDIALOG:
		iMethod = 0;
		bAuto = 1;
		bQuit = 1;

		SetWindowTextA( hDlg, APP_NAME );
		SetDlgItemText( hDlg, IDC_EXE, "example.exe" );
		SendMessage( hDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIcon( g_hInstance, MAKEINTRESOURCE( IDI_ICON ) ) );
		SendMessage( hDlg, WM_SETICON, ICON_SMALL, ( LPARAM )LoadIcon( g_hInstance, MAKEINTRESOURCE( IDI_ICON ) ) );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"CreateRemoteThread" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"NtCreateThreadEx" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"RtlCreateUserThread" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"SetWindowsHookEx" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_ADDSTRING, NULL, ( LPARAM )"QueueUserAPC" );
		SendMessage( GetDlgItem( hDlg, IDC_METHOD ), CB_SETCURSEL, 0, NULL );
		SendMessage( GetDlgItem( hDlg, IDC_AUTO ), BM_SETCHECK, bAuto, NULL );
		SendMessage( GetDlgItem( hDlg, IDC_QUIT ), BM_SETCHECK, bQuit, NULL );
		EnableWindow( GetDlgItem( hDlg, IDC_QUIT ), false );
		EnableWindow( GetDlgItem( hDlg, IDC_INJECT ), false );

		return true;
	case WM_COMMAND:
		switch( wParam )
		{
		case IDC_ABOUT:
			DialogBoxA( NULL, MAKEINTRESOURCEA( IDD_ABOUT ), NULL, AboutDlgProc );
			break;
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
			break;
		case IDC_INJECT:
			{
				if( bIsProcessRunning( szExe ) && !IsNullOrEmpty( szDll[0] ) )
				{
					for( int i = 0; i < MAX_DLLS; i++ )
					{
						if( i == ( MAX_DLLS - 1 ) )
						{
							Sleep( 500 );
							if( bQuit )
								exit(0);
							else
								return true;
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
							WindowsHookInjection( GetProcessId( szExe ), szDll[i], i );
							break;
						case 4:
							APCInjection( GetProcessId( szExe ), szDll[i] );
							break;
						}
					}
				}
			}
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
					memset( szExe, 0, sizeof( szExe ) );
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
					if( iMethod == 3 )
					{
						DialogBoxA( NULL, MAKEINTRESOURCEA( IDD_FUNC ), NULL, FuncDlgProc );
					}
				}
			}
			break;
		case IDC_AUTO:
			{
				if( HIWORD( wParam ) == BN_CLICKED )
				{
					bAuto = !bAuto;
					if( !bAuto )
					{
						EnableWindow( GetDlgItem( hDlg, IDC_QUIT ), true );
						EnableWindow( GetDlgItem( hDlg, IDC_INJECT ), true );
					}
					else
					{
						SendMessage( GetDlgItem( hDlg, IDC_QUIT ), BM_SETCHECK, 1, NULL );
						bQuit = 1;
						EnableWindow( GetDlgItem( hDlg, IDC_QUIT ), false );
						EnableWindow( GetDlgItem( hDlg, IDC_INJECT ), false );
					}
				}
			}
			break;
		case IDC_QUIT:
			{
				if( HIWORD( wParam ) == BN_CLICKED )
				{
					bQuit = !bQuit;
				}
			}
			break;
		}
		return true;

	case WM_CLOSE:
		CloseHandle( hInjThread );
		EndDialog( hDlg, 0 );
		exit( 0 );
		return true;
	}
	return false;
}

vector<string> sFuncs;
LRESULT iFunc;

BOOL CALLBACK FuncDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	int iBuffer[MAX_DLLS];
	HWND hListBox = GetDlgItem( g_hWnd, IDC_LISTDLL );
	LRESULT dllsInBuffer = SendMessage( hListBox, LB_GETSELITEMS, MAX_DLLS, ( LPARAM )iBuffer );
	LRESULT iDll;

	switch( message )
	{
	case WM_INITDIALOG:
		{
			SendMessage( hDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIcon( g_hInstance, MAKEINTRESOURCE( IDI_ICON ) ) );
			SendMessage( hDlg, WM_SETICON, ICON_SMALL, ( LPARAM )LoadIcon( g_hInstance, MAKEINTRESOURCE( IDI_ICON ) ) );
			EnableWindow( g_hWnd, false );
			memset( iBuffer, 0, MAX_DLLS );

			for( int i = 0; i < ( int )dllsInBuffer; i++ )
			{
				if( IsNullOrEmpty( szDll[i] ) )
					break;
				SendMessage( GetDlgItem( hDlg, IDC_SDLLS ), LB_ADDSTRING, NULL, ( LPARAM )szDll[i] );
			}
		}
		return true;

	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDC_FOK:
			EnableWindow( g_hWnd, true );
			EndDialog( hDlg, 0 );
			break;
		case IDC_SDLLS:
			if( HIWORD( wParam ) == LBN_SELCHANGE )
			{
				sFuncs.clear();
				SendMessage( GetDlgItem( hDlg, IDC_FUNCS ), LB_RESETCONTENT, NULL, NULL );
				iDll = SendMessage( GetDlgItem( hDlg, IDC_SDLLS ), LB_GETCURSEL, NULL, NULL );

				GetDllFunctions( szDll[iDll], sFuncs );

				for( unsigned int i = 0; i < sFuncs.size(); i++ )
				{
					SendMessage( GetDlgItem( hDlg, IDC_FUNCS ), LB_ADDSTRING, NULL, ( LPARAM )sFuncs[i].c_str() );
				}
			}
			break;
		case IDC_FUNCS:
			if( HIWORD( wParam ) == LBN_SELCHANGE )
			{
				iFunc = SendMessage( GetDlgItem( hDlg, IDC_FUNCS ), LB_GETCURSEL, NULL, NULL );
				iDll = SendMessage( GetDlgItem( hDlg, IDC_SDLLS ), LB_GETCURSEL, NULL, NULL );
				sFuncs[iFunc].copy( szFuncName[iDll], sFuncs[iFunc].length(), 0 );
			}
		}
		break;
	case WM_CLOSE:
		EnableWindow( g_hWnd, true );
		EndDialog( hDlg, 0 );
		return true;
	}
	return false;
}

BOOL CALLBACK AboutDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_INITDIALOG:
		EnableWindow( g_hWnd, false );
		return true;

	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDOK:
			EnableWindow( g_hWnd, true );
			EndDialog( hDlg, 0 );
			break;
		}
		break;
	case WM_CLOSE:
		EnableWindow( g_hWnd, true );
		EndDialog( hDlg, 0 );
		return true;
	}
	return false;
}
