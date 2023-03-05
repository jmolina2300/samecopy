// samecopy.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

// Constants
#define FORMAT_ROBOCOPY_COMMAND "C:\\windows\\system32\\robocopy.exe %s"
const char *OPTIONS_TEXT = 
"-- ROBOCOPY OPTIONS --\r\n"
"/S\tCopy subdirectories (EXCLUDING empty directories).\r\n"
"\r\n"
"/E\tCopy subdirectories (INCLUDING empty directories).\r\n"
"\r\n"
"/LEV:<n>\tCopy only the top n levels of the source directory tree.\r\n"
"\r\n"
"/Z\tCopy files in restartable mode.\r\n"
"    \tIn restartable mode, should a file copy be interrupted, Robocopy can pick up\r\n"
"    \twhere it left off rather than recopying the entire file.\r\n"
"\r\n"
"/B\tCopy files in backup mode.\r\n"
"    \tBackup mode allows Robocopy to override file and folder\r\n"
"    \tpermission settings (ACLs). This allows you to copy files \r\n"
"    \tyou might otherwise not have access to, assuming it's being\r\n"
"    \trun under an account with sufficient privileges.\r\n"
"\r\n"
"/ZB\tCopy files in restartable mode.\r\n"
"  \tIf file access is denied, switches to backup mode.\r\n"
"\r\n"
"/J\tCopy using unbuffered I/O (recommended for large files).\r\n\r\n"
"/EFSRAW\tCopy all encrypted files in EFS RAW mode.\r\n"
"/COPY:<copyflags>\tSpecifies which file properties to copy.\r\n" 
"The valid values for this option are:\r\n"
"   D - Data\r\n"
"   A - Attributes\r\n"
"   T - Time stamps\r\n"
"   S - NTFS access control list (ACL)\r\n"
"   O - Owner information\r\n"
"   U - Auditing information\r\n"
"   Default value: DAT\r\n"
"/dcopy:<copyflags>\tSpecifies what to copy in directories. \r\n"
"The valid values for this option are:\r\n"
"   D - Data\r\n"
"   A - Attributes\r\n"
"   T - Time stamps\r\n"
"   Default value: DA\r\n"
"\r\n"
"/nodcopy  \tCopy no directory info (the default /dcopy:DA is done).\r\n"
"/nooffload\tCopy files without using the Windows Copy Offload mechanism.\r\n\r\n"
"Read more: https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/robocopy\r\n\r\n";

const DWORD HEIGHT_DEFAULT = 200;
DWORD heightOptions = 0;

DWORD widthDefault = 0;
DWORD heightDefault = 0;

WNDPROC OldTxtSrcProc;
WNDPROC OldTxtDestProc;
LRESULT CALLBACK TxtSrcProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TxtDestProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct
{
	LPSTR szSrcPath;
	LPSTR szDestPath;
	LPSTR szOptions;
} MYARGS;

MYARGS MyArgs = {NULL, NULL, NULL};

TCHAR error[MAX_PATH * 3];
TCHAR lpSrcPath[MAX_PATH + 1];
TCHAR lpDestPath[MAX_PATH + 1];
TCHAR lpOptions[MAX_PATH + 1];



BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID DoCopy(LPSTR srcPath, LPSTR destPath, LPSTR options);
BOOL GetDirectory(HWND hwnd, LPSTR szDir);
BOOL GetDirectoryNT6(HWND hwnd, LPSTR szDir);  // Windows Vista+ version
BOOL GetDirectoryNT5(HWND hwnd, LPSTR szDir);  // Windows XP version
VOID GetDropFileName(HDROP hDrop , HWND hwnd);
BOOL GetPathFromDroppedFile();
VOID InitializeDialog(HWND hDlg);
VOID confirmCopy(HWND hDlg);
VOID DoCopyThreadRoutine(MYARGS *args);
VOID GetDialogDimensions(HWND hDlg, DWORD *, DWORD *);
VOID ToggleOptions(HWND hDlg);




int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	BOOL ret;
	HWND hdlg;
	MSG msg;
	LoadLibrary(TEXT("Msftedit.dll"));
	LoadLibrary(TEXT("Riched20.dll"));

	// Create the dialog box
	hdlg = CreateDialog(hInstance, 
						MAKEINTRESOURCE(IDD_MAIN_DIALOG),
						NULL, 
						DlgProc);
	if (hdlg == INVALID_HANDLE_VALUE) 
	{
		return 0;
	}
	ShowWindow(hdlg, nCmdShow);
    while((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
		if (ret == -1) {
			return -1;
		}
		if (!IsDialogMessage(hdlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }
    return msg.wParam;
}


BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR szDirTmp[MAX_PATH + 1];
	switch(uMsg)
	{
	case WM_INITDIALOG: 
		InitializeDialog(hDlg);
		return TRUE; 
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BTN_PATH_SOURCE:
			if (GetDirectory(hDlg, szDirTmp)) {
				SetDlgItemText(hDlg, IDC_TXT_SOURCE, szDirTmp);
			}		
			return TRUE;
		case IDC_BTN_PATH_DEST:
			if (GetDirectory(hDlg, szDirTmp)) {
				SetDlgItemText(hDlg, IDC_TXT_DEST, szDirTmp);
			}
			return TRUE;
		case IDC_BTN_OPTIONS:
			ToggleOptions(hDlg);
			return TRUE;
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;
		case IDOK:
			confirmCopy(hDlg);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hDlg);
		return TRUE;

	case WM_DESTROY:
		DragAcceptFiles(hDlg, FALSE);
		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}


/**
 * @brief Toggle Options help text
 * 
 * Expands the dialog box vertically to show the help text.
 * 
 * @param hDlg 
 * @return VOID 
 */
VOID ToggleOptions(HWND hDlg)
{
	RECT rect;
	GetWindowRect(hDlg, &rect);
	long currentHeight = rect.bottom - rect.top;
	long currentWidth = rect.right - rect.left;

	if (currentHeight == heightDefault) {
		// Show the options text
		SetWindowPos(hDlg, NULL, 0, 0, currentWidth, heightOptions, SWP_NOMOVE | SWP_NOZORDER);
		ShowWindow(GetDlgItem(hDlg, IDC_TXT_OPTIONS_HELP), SW_SHOW);

	} else {
		// Hide the options text
		SetWindowPos(hDlg, NULL, 0, 0, currentWidth, heightDefault, SWP_NOMOVE | SWP_NOZORDER);
		ShowWindow(GetDlgItem(hDlg, IDC_TXT_OPTIONS_HELP), SW_HIDE);
	}
}



VOID confirmCopy(HWND hDlg) 
{
	int iConfirm;
	int iSrcPathLength = SendDlgItemMessage(hDlg, IDC_TXT_SOURCE, WM_GETTEXTLENGTH, 0, 0);
	int iDestPathLength = SendDlgItemMessage(hDlg, IDC_TXT_DEST, WM_GETTEXTLENGTH, 0, 0);
	int iOptionsLength = SendDlgItemMessage(hDlg, IDC_TXT_OPTIONS, WM_GETTEXTLENGTH, 0, 0);
	if (!iSrcPathLength || !iDestPathLength) {
		return;
	}
	if (iSrcPathLength >= MAX_PATH || iDestPathLength >= MAX_PATH || iOptionsLength >= MAX_PATH) {
		MessageBox(hDlg, "Path exceeds MAX_PATH (260 characters)!", "Error", MB_OK);
		return;
	}


	// Get the Source/Dest path strings from txt boxes
	SendDlgItemMessage(hDlg, IDC_TXT_SOURCE, WM_GETTEXT, (WPARAM)MAX_PATH + 1, (LPARAM)lpSrcPath);
	SendDlgItemMessage(hDlg, IDC_TXT_DEST, WM_GETTEXT, (WPARAM)MAX_PATH + 1, (LPARAM)lpDestPath);
	SendDlgItemMessage(hDlg, IDC_TXT_OPTIONS, WM_GETTEXT, (WPARAM)MAX_PATH + 1, (LPARAM)lpOptions);

	// Confirm the copy
	sprintf(error, "Copy %s\nto\n%s?", lpSrcPath, lpDestPath);
	iConfirm = MessageBox(hDlg, error , "Confirm", MB_YESNO | MB_ICONINFORMATION);
	if (iConfirm == IDYES) 
	{
		// Create a thread to run RoboCopy.exe
		MyArgs.szSrcPath = lpSrcPath;
		MyArgs.szDestPath = lpDestPath;
		MyArgs.szOptions = lpOptions;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DoCopyThreadRoutine, &MyArgs, 0, NULL);
	}
}

VOID DoCopyThreadRoutine(MYARGS *args) {
	DoCopy(args->szSrcPath, args->szDestPath, args->szOptions);
}

VOID DoCopy(LPSTR lpSourcePath, LPSTR lpDestinationPath, LPSTR lpOptions)
{
	TCHAR error[120];
	TCHAR parameters[MAX_PATH * 2]; 
	TCHAR commandLine[MAX_PATH * 3];
    STARTUPINFO          si;
    PROCESS_INFORMATION  pi;	
	DWORD                ec;

	// TODO: check string for illegal characters before copying the path 
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
	
	// Setup the command line with the path names in quotes
	sprintf(parameters, "\"%s\" \"%s \" %s", 
								lpSourcePath, 
								lpDestinationPath, 
								lpOptions);
	sprintf(commandLine, FORMAT_ROBOCOPY_COMMAND , parameters);
	
    // Start the child process. 
    if( !CreateProcess(
		NULL,
        commandLine,
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
		sprintf(error,"Your computer is missing robocopy.exe!"); 
		MessageBox(NULL,error, "Error", MB_OK | MB_ICONEXCLAMATION);
        return;
    }
	
    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );
	
	// Handle errors (more info at https://ss64.com/nt/robocopy-exit.html)
	GetExitCodeProcess(pi.hProcess, &ec);
	CloseHandle( pi.hThread );   // Close thread handle
    CloseHandle( pi.hProcess );  // Close process handle
	if (ec & 16) {
		sprintf(error,"No files were copied. Check the log file for information.", ec);
		MessageBox(NULL, error, "Fatal Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	else if (ec & 8) {
		sprintf(error, "Some files or directories could not be copied (copy errors occurred and the retry limit was exceeded)", ec);
		MessageBox(NULL, error, "Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	else if (ec & 4) {
		sprintf(error, "Some Mismatched files or directories were detected", ec);
		MessageBox(NULL, error, "Error", MB_OK | MB_ICONEXCLAMATION);
	}
	else if (ec & 2) {
		if (ec & 1) {
			sprintf(error, "Some files were copied. Additional files were present in the destination folder. No failure was encountered.", ec);
			MessageBox(NULL, error, "Error", MB_OK | MB_ICONEXCLAMATION);
		} else {
			sprintf(error, "No files were copied. Some extra files or directories were present in the destination folder", ec);
			MessageBox(NULL, error, "Error", MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}

	int confirm = MessageBox(NULL, "Copy complete. Would you like to open the new folder?", "Info", MB_YESNO | MB_ICONINFORMATION);
	if (confirm == IDYES) {
		ShellExecute(NULL, "open", lpDestinationPath, NULL, NULL, SW_RESTORE);
	}
}


/**
 * @brief Get the Drop File Name 
 * 
 * Retrieves the file name of an HDROP object.
 * This is used by the editBox controls in their window procedures.
 * 
 * @param hDrop 
 * @param hwnd 
 */
VOID GetDropFileName(HDROP hDrop , HWND hwnd)
{
	TCHAR buf[MAX_PATH];
	DragQueryFile(hDrop, 0, buf, MAX_PATH);

	SetWindowText(hwnd, buf);
	DragFinish(hDrop);
}
LRESULT CALLBACK TxtSrcProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
	case WM_DROPFILES:
		GetDropFileName((HDROP)wParam, hwnd);
		return 0;
	}
	return CallWindowProc((WNDPROC)OldTxtSrcProc, hwnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK TxtDestProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
	case WM_DROPFILES:
		GetDropFileName((HDROP)wParam, hwnd);
		return 0;
	}
	return CallWindowProc((WNDPROC)OldTxtDestProc, hwnd, uMsg, wParam, lParam);
}


VOID InitializeDialog(HWND hDlg)
{
	HWND hwndOwner; 
	RECT rc, rcDlg, rcOwner; 
    HICON hIcon;
    hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
        MAKEINTRESOURCEW(IDI_ICON_LARGE),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        0);
    if (hIcon)
    {
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }
    hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
        MAKEINTRESOURCEW(IDI_ICON_SMALL),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        0);
    if (hIcon)
    {
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

	// BEGIN Center Dialog Window (code from microsoft)
	if ((hwndOwner = GetParent(hDlg)) == NULL) 
	{
		hwndOwner = GetDesktopWindow(); 
	}

	GetWindowRect(hwndOwner, &rcOwner); 
	GetWindowRect(hDlg, &rcDlg); 
	CopyRect(&rc, &rcOwner); 
	

	// Offset the owner and dialog box rectangles so that right and bottom 
	// values represent the width and height, and then offset the owner again 
	// to discard space taken up by the dialog box. 

	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
	OffsetRect(&rc, -rc.left, -rc.top); 
	OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

	// The new position is the sum of half the remaining space and the owner's 
	// original position. 
	SetWindowPos(hDlg, 
				 HWND_TOP, 
				 rcOwner.left + (rc.right / 2), 
				 rcOwner.top + (rc.bottom / 2), 
				 0, 0,          // Ignores size arguments. 
				 SWP_NOSIZE); 
	// END Center Dialog Window
	
	SetFocus(GetDlgItem(hDlg, IDOK)); 
	SetWindowText(GetDlgItem(hDlg, IDC_TXT_OPTIONS), "/E ");
	
	// Set old WndProcs for the edit controls
	OldTxtSrcProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, IDC_TXT_SOURCE), GWLP_WNDPROC, (LONG_PTR)TxtSrcProc);
	OldTxtDestProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, IDC_TXT_DEST), GWLP_WNDPROC, (LONG_PTR)TxtDestProc);


	/* Set Default Window Dimensions 
	 *
	 * The default height we WANT is 200, but the window starts out bigger 
	 * because in the dialog editor, the main window must be big enough to fit 
	 * the Edit control for the options text.
	 *
	 * Thus, we perform the following procedure:
	 * 
	 *  1. get the width and height values at startup
	 *  2. save the width and height values
	 *  3. set the height to HEIGHT_DEFAULT (currently 200)
	 *  4. apply new dimensions to main window
	 *  
	 */
	{
	DWORD heightAtStartup = 0;
	DWORD widthAtStartup = 0;
	GetDialogDimensions(hDlg, &widthAtStartup, &heightAtStartup);
	widthDefault = widthAtStartup;
	heightOptions = heightAtStartup;
	heightDefault = HEIGHT_DEFAULT;
	}
	SetWindowPos(hDlg, HWND_TOP, 0, 0, widthDefault, heightDefault, SWP_NOMOVE);

	// Fill the editBox with the options help text
	SetWindowText(GetDlgItem(hDlg, IDC_TXT_OPTIONS_HELP), OPTIONS_TEXT);

	DragAcceptFiles(hDlg, TRUE);
}


/**
 * @brief Get the Dialog Dimensions 
 * 
 * Called at startup: Assigns the width and height reference variables
 * 
 * @param hDlg 
 * @param width 
 * @param height 
 * @return VOID 
 */
VOID GetDialogDimensions(HWND hDlg, DWORD *width, DWORD *height)
{
	RECT rc;
	GetWindowRect(hDlg, &rc);
	*width = rc.right - rc.left;
	*height = rc.bottom - rc.top;
}



/**
 * @brief Get the Directory 
 * 
 * Selects the appropriate GetDirectory function based on
 * the OS version.
 * 
 * @param hwnd 
 * @param szDir 
 * @return BOOL 
 */
BOOL GetDirectory(HWND hwnd, LPSTR szDir)
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if (osvi.dwMajorVersion >= 6) {
		return GetDirectoryNT6(hwnd, szDir);
	} else {
		return GetDirectoryNT5(hwnd, szDir);
	}

}


/**
 * @brief Copy PWSTR to LPSTR
 * 
 */
VOID CopyPWSTRtoLPSTR(LPSTR dest, PWSTR src)
{
	int i = 0;
	while (src[i] != 0) {
		dest[i] = (TCHAR)src[i];
		i++;
	}
	dest[i] = 0;
}



/**
 * @brief GetDirectory (Windows XP)
 * 
 * Opens a folder browser dialog and returns the selected folder
 * 
 * @param hwnd 
 * @param szDir 
 * @return BOOL 
 */
BOOL GetDirectoryNT5(HWND hwnd, LPSTR szDir)
{
	BROWSEINFO bInfo;
	bInfo.hwndOwner = hwnd;
	bInfo.pidlRoot = NULL; 
	bInfo.pszDisplayName = szDir;
	bInfo.lpszTitle = "Please, select a folder";
	bInfo.ulFlags = 0 ;
	bInfo.lpfn = NULL;
	bInfo.lParam = 0;
	bInfo.iImage = -1;

	LPITEMIDLIST lpItem = SHBrowseForFolder( &bInfo);
	if( lpItem != NULL )
	{
	  SHGetPathFromIDList(lpItem, szDir );
	  return TRUE;
	}
	return FALSE;
}

/**
 * @brief GetDirectory (Windows 7+ )
 * 
 * Opens a folder browser dialog and returns the selected folder
 * 
 * @param hwnd 
 * @param filePathBuffer 
 * @return BOOL 
 */
BOOL GetDirectoryNT6(HWND hwnd, LPSTR filePathBuffer)
{
	IFileDialog *pfd = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

	// Open the dialog
	if (SUCCEEDED(hr))
	{
		DWORD dwOptions;
		if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
		{
			pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
		}
		hr = pfd->Show(hwnd);
		if (SUCCEEDED(hr))
		{
			IShellItem *psiResult;
			hr = pfd->GetResult(&psiResult);
			if (SUCCEEDED(hr))
			{
				PWSTR pszFilePath;
				hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (SUCCEEDED(hr))
				{
					/**
					 * Convert the file path from PWSTR to LPSTR
					 * 
					 * This could be bad for two reasons:
					 *  1. filePathBuffer is too small
					 *  2. characters are lost in the conversion
					 */
					CopyPWSTRtoLPSTR(filePathBuffer, pszFilePath);
					CoTaskMemFree(pszFilePath);
				}
				psiResult->Release();
			}
		}
		pfd->Release();
	}
	return SUCCEEDED(hr);
}
