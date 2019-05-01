#include "dllmain.h"
#include "Game.h"
#include <iostream>
#include <strsafe.h>

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

BOOL ApplyHooks()
{
	bool blockNetworkAccess = (GetPrivateProfileIntW(L"online", L"blockNetworkAccess", 1, L".\\modengine.ini") == 1);
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
	if (GetGameType() == GAME_DARKSOULS_3 && !BypassHideThreadFromDebugger())
		throw(0xDEAD0002);

	// Bypass AssemblyValidation
	//if (!BypassAssemblyValidation())
	//	throw(0xDEAD0003);

	// Patch for loose params
	if (!LooseParamsPatch(saveFilePatch, looseParamsPatch))
		throw(0xDEAD0003);

	// Block network access
	if (GetGameType() != GAME_SEKIRO && blockNetworkAccess)
	{
		if (!BlockNetworkConnection())
			throw(0xDEAD0003);
	}

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

	return true;
}

void LoadPlugins()
{
	//LoadLibraryW(L".\\plugins\\SekiroTutorialRemover.dll");
}

// SteamAPI hook
typedef DWORD64(__cdecl *STEAMINIT)();
STEAMINIT fpSteamInit = NULL;
DWORD64 __cdecl onSteamInit()
{
	// Check Sekiro version
	if ((GetGameType() == GAME_SEKIRO) && !CheckSekiroVersion())
	{
		AllocConsole();
		FILE *stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONIN$", "r", stdin);
		printf("Unsupported version of Sekiro detected. This version of Mod Engine was built for Sekiro 1.02-1.03 official steam release, and is not supported with cracks or other versions.\r\nIf Steam updated your game recently, check for the latest mod engine version at https://www.nexusmods.com/sekiro/mods/6.\r\n");
		printf("\r\nMod Engine will attempt to find the required functions in order to work, but I give no guarantees.\r\nDO NOT ASK ME FOR SUPPORT IF THINGS DON'T WORK PROPERLY.\r\n\r\nPress any key to continue...");
		int temp;
		std::cin.ignore();
		FreeConsole();
	}

	ApplyHooks();
	LoadPlugins();
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
    
	// Check the DkS3 version
	if ((GetGameType() == GAME_DARKSOULS_3) && !CheckDkSVersion())
		throw(0xDEAD0000);

	// Initialize MinHook
	if (MH_Initialize() != MH_OK)
		throw(0xDEAD0001);

	// Only hook steamapi on Sekiro
	if (GetGameType() == GAME_SEKIRO || GetGameType() == GAME_DARKSOULS_REMASTERED)
	{
		auto steamApiHwnd = GetModuleHandleW(L"steam_api64.dll");
		auto initAddr = GetProcAddress(steamApiHwnd, "SteamAPI_Init");
		MH_CreateHook(initAddr, &onSteamInit, reinterpret_cast<LPVOID*>(&fpSteamInit));
		MH_EnableHook(initAddr);
	}
	else
	{
		// Just call our would-be steam hook directly since exe isn't Steam DRM protected
		onSteamInit();
	}

    return true;
}

BOOL ExitInstance()
{
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (GetPrivateProfileIntW(L"debug", L"showDebugLog", 0, L".\\modengine.ini") == 1)
	{
		AllocConsole();
		FILE *stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		gDebugLog = true;
	}

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        InitInstance(hModule);
        break;
    case DLL_PROCESS_DETACH:
        ExitInstance();
        break;
    }
    return TRUE;
}

