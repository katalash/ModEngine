#include "HideThreadFromDebugger.h"

typedef BOOL (WINAPI *ZWSETINFORMATIONTHREAD)(HANDLE, THREAD_INFORMATION_CLASS, PVOID, ULONG);

// Pointer for calling original function
ZWSETINFORMATIONTHREAD fpZwSetInformationThread = NULL;

// Detour function
INT WINAPI tZwSetInformationThread(HANDLE threadHandle, THREAD_INFORMATION_CLASS threadInfoClass, PVOID threadInfo, ULONG threadInfoLength)
{
	if (threadInfoClass == 0x11) // ThreadHideFromDebugger
	{
		return 0x1; // return STATUS_SUCCESS as if we set the ThreadHideFromDebugger flag
	}

	return fpZwSetInformationThread(threadHandle, threadInfoClass, threadInfo, threadInfoLength); // return the original function if any other info class
}

BOOL BypassHideThreadFromDebugger()
{

	if (MH_CreateHookApi(L"ntdll", "NtSetInformationThread", &tZwSetInformationThread, reinterpret_cast<LPVOID*>(&fpZwSetInformationThread)) != MH_OK)
		return false;

	if (MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleW(L"ntdll"), "NtSetInformationThread")) != MH_OK)
		return false;
	
	return true;
}