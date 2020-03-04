#include "dllmain.h"
#include "Game.h"
#include "d3d11hook.h"
#include "StackWalker/StackWalker.h"
#include <iostream>
#include <strsafe.h>
#include <stdio.h>
#include <tchar.h>


// Export DINPUT8
tDirectInput8Create oDirectInput8Create;

bool gDebugLog = false;

BOOL CheckDkSVersion()
{
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);

	FILE *p_file = nullptr;
	fopen_s(&p_file, buffer, "rb");
	fseek(p_file, 0, SEEK_END);
	long size = ftell(p_file);
	fclose(p_file);

	// 1.15 = 102494368
	if (size == (long)102494368)
	{
		return true;
	}
	else
	{
		return false;
	}
}

BOOL CheckSekiroVersion()
{
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);

	FILE *p_file = nullptr;
	fopen_s(&p_file, buffer, "rb");
	fseek(p_file, 0, SEEK_END);
	long size = ftell(p_file);
	fclose(p_file);

	// 1.02 = 65682008
	// 1.02 unpacked = 65682312
	// 1.03 = 65688152
	if (size == (long)65682008 || size == (long)65682312 || size == (long)65688152)
	{
		// Check for CODEX crack
		wchar_t buffer2[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH * 2, buffer2);
		StringCchCatW(buffer2, MAX_PATH, L"\\sekiro.cdx");
		if (GetFileAttributesW(buffer2) != INVALID_FILE_ATTRIBUTES)
		{
			// CODEX crack detected
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}

void LoadPlugins()
{
	//LoadLibraryW(L".\\plugins\\SekiroTutorialRemover.dll");
}

BOOL ApplyPostUnpackHooks()
{
	// Check Sekiro version
	if ((GetGameType() == GAME_SEKIRO) && !CheckSekiroVersion())
	{
		AllocConsole();
		FILE* stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONIN$", "r", stdin);
		printf("Unsupported version of Sekiro detected. This version of Mod Engine was built for Sekiro 1.02-1.03 official steam release, and is not supported with cracks or other versions.\r\nIf Steam updated your game recently, check for the latest mod engine version at https://www.nexusmods.com/sekiro/mods/6.\r\n");
		printf("\r\nMod Engine will attempt to find the required functions in order to work, but I give no guarantees.\r\nDO NOT ASK ME FOR SUPPORT IF THINGS DON'T WORK PROPERLY.\r\n\r\nPress any key to continue...");
		int temp;
		std::cin.ignore();
		FreeConsole();
	}

	// Check the DkS3 version
	if ((GetGameType() == GAME_DARKSOULS_3) && !CheckDkSVersion())
	{
		AllocConsole();
		FILE* stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONIN$", "r", stdin);
		printf("Unsupported version of Dark Souls 3 detected. This version of Mod Engine was built for DS3 App Version 1.15 official steam release, and is not supported with cracks or other versions.");
		printf("\r\nMod Engine will attempt to find the required functions in order to work, but I give no guarantees.\r\nDO NOT ASK ME FOR SUPPORT IF THINGS DON'T WORK PROPERLY.\r\n\r\nPress any key to continue...");
		int temp;
		std::cin.ignore();
		FreeConsole();
	}

	// Break on startup if specified
	bool startupBreak = (GetPrivateProfileIntW(L"debug", L"breakOnStart", 0, L".\\modengine.ini") == 1);
	if (startupBreak)
	{
		printf("Startup attach point.\r\n\r\nPress any key to continue...");
		int temp;
		std::cin.ignore();
	}

	bool saveFilePatch = (GetPrivateProfileIntW(L"savefile", L"useAlternateSaveFile", 1, L".\\modengine.ini") == 1);
	bool looseParamsPatch = (GetPrivateProfileIntW(L"files", L"loadLooseParams", 0, L".\\modengine.ini") == 1);
	bool loadUXMFiles = (GetPrivateProfileIntW(L"files", L"loadUXMFiles", 0, L".\\modengine.ini") == 1);
	bool useModOverride = (GetPrivateProfileIntW(L"files", L"useModOverrideDirectory", 1, L".\\modengine.ini") == 1);
	bool cachePaths = (GetPrivateProfileIntW(L"files", L"cacheFilePaths", 1, L".\\modengine.ini") == 1);

	wchar_t *modOverrideDirectory = (wchar_t*)malloc(1000);
	if (modOverrideDirectory != NULL)
	{
		GetPrivateProfileStringW(L"files", L"modOverrideDirectory", L"\\doa", modOverrideDirectory, 500, L".\\modengine.ini");
	}

	// Bypass HideThreadFromDebugger
	if ((GetGameType() == GAME_DARKSOULS_3 || GetGameType() == GAME_DARKSOULS_2_SOTFS) && !BypassHideThreadFromDebugger())
		throw(0xDEAD0002);

	// Bypass AssemblyValidation
	//if (!BypassAssemblyValidation())
	//	throw(0xDEAD0003);

	// Patch for loose params
	if (!LooseParamsPatch(saveFilePatch, looseParamsPatch))
		throw(0xDEAD0003);

	// Mod loader
	if (!HookModLoader(loadUXMFiles, useModOverride, cachePaths, modOverrideDirectory))
	{
		AllocConsole();
		FILE *stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONIN$", "r", stdin);
		printf("Failed to find required game code to hook. This version of Mod Engine was built for Sekiro 1.02 official steam release, and may not compatible with cracks or other versions. If Steam updated your game recently, check for the latest mod engine version at https://www.nexusmods.com/sekiro/mods/6.\r\n");
		printf("\r\nMod Engine will be disabled because it can't hook in a stable way. Press any key to continue...");
		int temp;
		std::cin.ignore();
		FreeConsole();
		return true;
	}

	if (GetGameType() == GAME_DARKSOULS_3 && !ApplyGameplayPatches())
		throw(0xDEAD0004);

	if (!ApplyMiscPatches())
		throw(0xDEAD0004);

	LoadPlugins();

	return true;
}

// SteamAPI hook
typedef DWORD64(__cdecl *STEAMINIT)();
STEAMINIT fpSteamInit = NULL;
DWORD64 __cdecl onSteamInit()
{
	ApplyPostUnpackHooks();
	return fpSteamInit();
}

BOOL InitInstance(HMODULE hModule)
{
    // Load the real dinput8.dll, or chain load another dll injection
    HMODULE hMod = NULL;
	wchar_t dllPath[MAX_PATH];
	wchar_t chainPath[MAX_PATH];

	GetPrivateProfileStringW(L"misc", L"chainDInput8DLLPath", L"", chainPath, MAX_PATH, L".\\modengine.ini");

	if (lstrlenW(chainPath) > 0)
	{
		GetCurrentDirectoryW(MAX_PATH, dllPath);
		lstrcatW(dllPath, chainPath);
		wprintf(L"[Mod Engine] Attempting to chain load DLL %s\r\n", dllPath);
		hMod = LoadLibraryW(dllPath);
		if (hMod != NULL)
		{
			oDirectInput8Create = (tDirectInput8Create)GetProcAddress(hMod, "DirectInput8Create");
			wprintf(L"[Mod Engine] Chain load successful\r\n");
		}
		else
		{
			wprintf(L"[Mod Engine] Chain load failed. Falling back to system dinput8.dll\r\n");
		}
	}
	if (hMod == NULL)
	{
		GetSystemDirectoryW(dllPath, MAX_PATH);
		lstrcatW(dllPath, L"\\dinput8.dll");
		hMod = LoadLibraryW(dllPath);
		oDirectInput8Create = (tDirectInput8Create)GetProcAddress(hMod, "DirectInput8Create");
	}

	// Initialize MinHook
	if (MH_Initialize() != MH_OK)
	{
		AllocConsole();
		FILE* stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONIN$", "r", stdin);
		printf("Fatal Error: Minhook failed to initialize\r\n");
		int temp;
		std::cin.ignore();
		FreeConsole();
		throw(0xDEAD0003);
	}

	// Do early hook of WSA stuff
	bool blockNetworkAccess = (GetPrivateProfileIntW(L"online", L"blockNetworkAccess", 1, L".\\modengine.ini") == 1);
	if (GetGameType() != GAME_SEKIRO && blockNetworkAccess)
	{
		if (!BlockNetworkConnection())
		{
			AllocConsole();
			FILE* stream;
			freopen_s(&stream, "CONOUT$", "w", stdout);
			freopen_s(&stream, "CONIN$", "r", stdin);
			printf("Fatal Error: Network blocking hook failed to attach\r\n");
			int temp;
			std::cin.ignore();
			FreeConsole();
			throw(0xDEAD0003);
		}
	}

	// Only hook steamapi on Sekiro
	if (GetGameType() == GAME_SEKIRO || GetGameType() == GAME_DARKSOULS_3 || GetGameType() == GAME_DARKSOULS_REMASTERED || GetGameType() == GAME_DARKSOULS_2_SOTFS)
	{
		auto steamApiHwnd = GetModuleHandleW(L"steam_api64.dll");
		auto initAddr = GetProcAddress(steamApiHwnd, "SteamAPI_Init");
		MH_CreateHook(initAddr, &onSteamInit, reinterpret_cast<LPVOID*>(&fpSteamInit));
		MH_EnableHook(initAddr);
	}
	else
	{
		// Just call our would-be steam hook directly since exe isn't Steam DRM protected
		//onSteamInit();
	}

	// DS2 light params. Lighting is done here since directional shadow map is initialized super early
	int dirRes = GetPrivateProfileIntW(L"rendering", L"directionalShadowResolution", 4096, L".\\modengine.ini") / 2;
	int atlasRes = GetPrivateProfileIntW(L"rendering", L"dynamicAtlasShadowResolution", 2048, L".\\modengine.ini") / 2;
	int pointRes = GetPrivateProfileIntW(L"rendering", L"dynamicPointShadowResolution", 512, L".\\modengine.ini") / 2;
	int spotRes = GetPrivateProfileIntW(L"rendering", L"dynamicSpotShadowResolution", 1024, L".\\modengine.ini") / 2;

	if (GetGameType() == GAME_DARKSOULS_2_SOTFS && !ApplyShadowMapResolutionPatches(dirRes, atlasRes, pointRes, spotRes))
		throw(0xDEAD0004);

    return true;
}

BOOL ExitInstance()
{
    return true;
}

const LPCWSTR AppWindowTitleDS3 = L"DARK SOULS III"; // Targeted D11 Application Window Title.
const LPCWSTR AppWindowTitleSekiro = L"Sekiro"; // Targeted D11 Application Window Title.

DWORD WINAPI MainThread(HMODULE hModule)
{
	//Sleep(1000);
	ApplyDS3SekiroAllocatorLimitPatch();
	if (GetGameType() == GAME_DARKSOULS_3)
	{
		while (FindWindowW(0, AppWindowTitleDS3) == NULL)
		{

		}
	}
	if (GetGameType() == GAME_SEKIRO)
	{
		HWND window = NULL;
		char title[255];
		while (window == NULL)
		{
			window = FindWindowW(0, AppWindowTitleSekiro);
			RECT rect;
			GetWindowRect(window, &rect);
			if (!IsIconic(window))
			{
				window = NULL;
			}
			BOOL vis = IsWindowEnabled(window);
			printf("Not found window? vis %d\n", vis);
		}

		GetWindowTextA(window, title, 255);

		printf("Found window %s?\n", title);
	}
	//bool s = ImplHookDX11_Init(hModule, FindWindowW(0, AppWindowTitle));
	//if (!s)
	//{
	//	wprintf(L"Hooking failed\n");
	//}

	ApplyPostUnpackHooks();

	return S_OK;
}

class StackWalkerToConsole : public StackWalker
{
protected:
	virtual void OnOutput(LPCSTR szText) { printf("%s", szText); }
};

class StackWalkerToFile : public StackWalker
{
	FILE* file;
public:
	StackWalkerToFile()
	{
		file = fopen("modenginecrash.log", "w");
	}

	void Close()
	{
		fclose(file);
	}

protected:
	virtual void OnOutput(LPCSTR szText) 
	{
		fprintf(file, "%s", szText);
		printf("%s", szText); 
	}
};

#if defined(_M_X64) || defined(_M_IX86)
static BOOL PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if (hKernel32 == NULL)
		return FALSE;
	void* pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if (pOrgEntry == NULL)
		return FALSE;

#ifdef _M_IX86
	// Code for x86:
	// 33 C0                xor         eax,eax
	// C2 04 00             ret         4
	unsigned char szExecute[] = { 0x33, 0xC0, 0xC2, 0x04, 0x00 };
#elif _M_X64
	// 33 C0                xor         eax,eax
	// C3                   ret
	unsigned char szExecute[] = { 0x33, 0xC0, 0xC3 };
#else
#error "The following code only works for x86 and x64!"
#endif

	DWORD dwOldProtect = 0;
	BOOL  bProt = VirtualProtect(pOrgEntry, sizeof(szExecute), PAGE_EXECUTE_READWRITE, &dwOldProtect);

	SIZE_T bytesWritten = 0;
	BOOL   bRet = WriteProcessMemory(GetCurrentProcess(), pOrgEntry, szExecute, sizeof(szExecute),
		&bytesWritten);

	if ((bProt != FALSE) && (dwOldProtect != PAGE_EXECUTE_READWRITE))
	{
		DWORD dwBuf;
		VirtualProtect(pOrgEntry, sizeof(szExecute), dwOldProtect, &dwBuf);
	}
	return bRet;
}
#else
#pragma message("This code works only for x86 and x64!")
#endif

static TCHAR s_szExceptionLogFileName[_MAX_PATH] = _T("\\modenginecrash.log"); // default
static BOOL  s_bUnhandledExeptionFilterSet = FALSE;
static LONG __stdcall CrashHandlerExceptionFilter(EXCEPTION_POINTERS* pExPtrs)
{
#ifdef _M_IX86
	if (pExPtrs->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		static char MyStack[1024 * 128]; // be sure that we have enough space...
		// it assumes that DS and SS are the same!!! (this is the case for Win32)
		// change the stack only if the selectors are the same (this is the case for Win32)
		//__asm push offset MyStack[1024*128];
		//__asm pop esp;
		__asm mov eax, offset MyStack[1024 * 128];
		__asm mov esp, eax;
	}
#endif

	StackWalkerToFile sw; // output to console
	sw.ShowCallstack(GetCurrentThread(), pExPtrs->ContextRecord);
	sw.Close();
	TCHAR lString[500];
	_stprintf_s(lString,
		_T("*** Unhandled Exception! See console output for more infos!\n")
		_T("   ExpCode: 0x%8.8X\n")
		_T("   ExpFlags: %d\n")
#if _MSC_VER >= 1900
		_T("   ExpAddress: 0x%8.8p\n"),
#else
		_T("   ExpAddress: 0x%8.8X\n")
#endif
		pExPtrs->ExceptionRecord->ExceptionCode, pExPtrs->ExceptionRecord->ExceptionFlags,
		pExPtrs->ExceptionRecord->ExceptionAddress);
	FatalAppExit(-1, lString);
	return EXCEPTION_CONTINUE_SEARCH;
}

static void InitUnhandledExceptionFilter()
{
	TCHAR szModName[_MAX_PATH];
	if (GetModuleFileName(NULL, szModName, sizeof(szModName) / sizeof(TCHAR)) != 0)
	{
		_tcscpy_s(s_szExceptionLogFileName, szModName);
		_tcscat_s(s_szExceptionLogFileName, _T(".exp.log"));
	}
	if (s_bUnhandledExeptionFilterSet == FALSE)
	{
		// set global exception handler (for handling all unhandled exceptions)
		SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
#if defined _M_X64 || defined _M_IX86
		PreventSetUnhandledExceptionFilter();
#endif
		s_bUnhandledExeptionFilterSet = TRUE;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	InitUnhandledExceptionFilter();

	if (GetPrivateProfileIntW(L"debug", L"showDebugLog", 0, L".\\modengine.ini") == 1)
	{
		AllocConsole();
		FILE *stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONIN$", "r", stdin);
		gDebugLog = true;
	}

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //DisableThreadLibraryCalls(hModule);
        InitInstance(hModule);
		ApplyAllocatorLimitPatchVA();
		if (GetGameType() == GAME_DARKSOULS_3)
		{
			// Experimental threaded patcher
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainThread, hModule, NULL, NULL);
		}
        break;
    case DLL_PROCESS_DETACH:
        ExitInstance();
        break;
    }
    return TRUE;
}

