#include "NetworkBlocker.h"
#include <stdio.h>
#include <iostream>
//#include <WinSock2.h>

typedef int (__stdcall *WSASTARTUP)(WORD, void*);

// Pointer for calling original function
WSASTARTUP fpWsaStartup = NULL;

// block windows sockets from ever being initialized
INT __stdcall tWSAStartup(WORD wVersionRequested, void* lpWSAData)
{
	//wprintf(L"[ModEngine] WSAStartup called\r\n");
	return 10091L; // WSASYSNOTREADY
}

BOOL BlockNetworkConnection()
{
	wprintf(L"[ModEngine] Blocking winsocket startup\r\n");
	if (MH_CreateHookApi(L"ws2_32", "WSAStartup", &tWSAStartup, reinterpret_cast<LPVOID*>(&fpWsaStartup)) != MH_OK)
		return false;

	if (MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleW(L"ws2_32"), "WSAStartup")) != MH_OK)
		return false;

	return true;
}