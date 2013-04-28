#include "windows.h"
#include <iostream>
#include <string>
#pragma comment(lib, "Advapi32.lib")
using namespace std;
char* sendString(HANDLE hproc, char* str)
{
	int len = strlen(str) + 1;
	char* addr = (char*)VirtualAllocEx(hproc, NULL, len, MEM_COMMIT, PAGE_READWRITE);
	DWORD numBytesWritten;
	WriteProcessMemory(hproc, addr, str, len, &numBytesWritten);
	return addr;
}
int main(int argc, char** argv)
{
	DWORD pid;
	DWORD scratch;

	cout << "pid: ";
	cin >> pid;

	// afaik this is required ever since installing GameGuard
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, 0, &tkp, sizeof(tkp), NULL, NULL);
	}

	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if(!process)
	{
		cout << "Couldn't open process pid " << pid << endl;
		return 1;
	}

	char* rmodulename = sendString(process, "E:\\Documents and Settings\\phree\\My Documents\\Visual Studio 2005\\Projects\\apilog\\release\\apilog.dll");
	//char* rmodulename = sendString(process, "E:\\downloads\\vivox-SLVoice-2.1.3010.6270-Release-Win32_2\\vivox-sdk\\lib\\vivoxsdk2.dll");

	HANDLE thread = CreateRemoteThread(process, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"), rmodulename, 0, 0);
	WaitForSingleObject(thread, INFINITE);

	CloseHandle(thread);
	VirtualFreeEx(process, rmodulename, 0, MEM_RELEASE);
	CloseHandle(process);
}
