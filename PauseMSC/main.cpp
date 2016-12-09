#include <windows.h>
#include <string>
#include <winternl.h>
#include <tlhelp32.h>

#pragma comment(lib,"comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <CommCtrl.h>
#include "resource.h"

#define STATUS_SUCCESS 0x00000000

typedef NTSTATUS(NTAPI *pdef_NtSuspendProcess)(HANDLE ProcessHandle); 
typedef NTSTATUS(NTAPI *pdef_NtResumeProcess)(HANDLE ProcessHandle);

HINSTANCE hInst;
bool paused = FALSE;
std::wstring prcStr = L"mysummercar.exe";
std::wstring btnStrs[] = { L"Pause Game" , L"Resume Game" };

BOOL CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
bool suspendProcess(DWORD targetP, bool suspend, HWND hwnd);
DWORD findPid(const std::wstring &str);
void logAdd(const std::wstring &str, HWND hwnd);



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProc);
}

BOOL CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			//init common controls
			INITCOMMONCONTROLSEX iccex;
			iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
			iccex.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;

			HICON hicon = static_cast<HICON>(LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE));
			SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);

			SendMessage(GetDlgItem(hwnd, IDC_TOGGLEPAUSE), WM_SETTEXT, 0, (LPARAM)btnStrs[paused].c_str());
			logAdd(L"Ready to boogie!", hwnd);
			logAdd(L"PauseMSC v1.0 - by durkhaz 2016", hwnd);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_TOGGLEPAUSE:
				{
					DWORD tpid = findPid(prcStr);
					bool bRet = (paused ? suspendProcess(tpid, false, hwnd) : suspendProcess(tpid, true , hwnd));
					if (bRet)
					{
						paused = !paused;
						SendMessage(GetDlgItem(hwnd, IDC_TOGGLEPAUSE), WM_SETTEXT, 0, (LPARAM)btnStrs[paused].c_str());
					}
				}
			}
			break;
		}
		case WM_CLOSE:
			if (paused)
			{
				DWORD tpid = findPid(prcStr);
				suspendProcess(tpid, false, hwnd);
			}
			EndDialog(hwnd, 0);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

DWORD findPid(const std::wstring &str)
{
	HANDLE ProcessHandle;
	PROCESSENTRY32 pEntry32;
	pEntry32.dwSize = sizeof(PROCESSENTRY32);
	ProcessHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	do
	{
		if (!wcscmp(pEntry32.szExeFile, str.c_str()))
		{
			CloseHandle(ProcessHandle);
			return pEntry32.th32ProcessID;
		}
	} while (Process32Next(ProcessHandle, &pEntry32));
	CloseHandle(ProcessHandle);
	return 010101;
}

bool suspendProcess(DWORD targetP, bool suspend, HWND hwnd)
{
	if (!targetP)
	{
		logAdd(L"Invalid PID.", hwnd);
		return false;
	}

	HANDLE ProcessHandle = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, targetP);

	if (!ProcessHandle)
	{
		logAdd(L"MSC isn't running.", hwnd);
		return false;
	}

	LPVOID lpFuncAddress = suspend ? GetProcAddress(LoadLibrary(L"ntdll.dll"), "NtSuspendProcess") : GetProcAddress(LoadLibrary(L"ntdll.dll"), "NtResumeProcess");

	if (!lpFuncAddress)
	{
		logAdd(suspend ? L"Failed to retrieve NtSuspendProcess" : L"Failed to retrieve NtResumeProcess", hwnd);
		return false;
	}

	NTSTATUS NtRet;
	if (suspend)
	{
		pdef_NtSuspendProcess NtCall = (pdef_NtSuspendProcess)lpFuncAddress;
		NtRet = NtCall(ProcessHandle);
	}
	else
	{
		pdef_NtResumeProcess NtCall = (pdef_NtResumeProcess)lpFuncAddress;
		NtRet = NtCall(ProcessHandle);
	}

	if (!NtRet == STATUS_SUCCESS)
	{
		return false;
	}
	logAdd(suspend ? L"Game was paused successfully!" : L"Game was resumed successfully!", hwnd);
	return true;
}

void logAdd(const std::wstring &str, HWND hwnd)
{
	int len = GetWindowTextLength(GetDlgItem(hwnd, IDC_OUTPUT)) + 1;
	unsigned int lines = SendMessage(GetDlgItem(hwnd, IDC_OUTPUT), EM_GETLINECOUNT, 0, 0);
	std::wstring buffer(len, '\0');
	GetWindowText(GetDlgItem(hwnd, IDC_OUTPUT), (LPTSTR)buffer.data(), len);

	if (lines > 5)
	{
		size_t firstNull = buffer.find_first_of(char(13));
		if (firstNull != std::wstring::npos)
		{
			buffer.erase(0, firstNull + 2);
		}
	}

	size_t firstNull = buffer.find_first_of(L'\0');
	if (firstNull != std::wstring::npos)
		buffer.resize(firstNull);

	SYSTEMTIME time;
	GetLocalTime(&time);

	wchar_t cbuf[128];
	memset(cbuf, 0, 128);
	swprintf(cbuf, 128, L"%02d:%02d - %s\r\n", time.wHour, time.wMinute, str.c_str());

	std::wstring newStr = cbuf;
	firstNull = newStr.find_first_of(L'\0');
	if (firstNull != std::wstring::npos)
		newStr.resize(firstNull);

	buffer += newStr;

	SetDlgItemText(hwnd, IDC_OUTPUT, buffer.c_str());
}