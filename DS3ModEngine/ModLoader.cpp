#include "ModLoader.h"
#include "Game.h"
#include "AOBScanner.h"
#include <stdio.h>
#include <wchar.h>
#include <Shlwapi.h>
#include <concurrent_unordered_set.h>

#pragma comment(lib, "shlwapi.lib")

extern bool gDebugLog;

typedef void*(*VIRTUALTOARCHIVEPATH)(DLString*, UINT64, UINT64, DLString*, UINT64, UINT64);

typedef void*(*FUCKSEKIRO)(SekiroString*, UINT64, UINT64, DLString*, UINT64, UINT64);

// Hook for create file syscall
typedef HANDLE(WINAPI *CREATEFILEW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

// Hook for Sekiro file loading function
typedef UINT64(*LOADFILE)(LPVOID, UINT64);

// Pointer for calling original function
VIRTUALTOARCHIVEPATH fpVirtualToArchivePath = NULL;
FUCKSEKIRO fpFuckSekiro = NULL;
CREATEFILEW fpCreateFileW = NULL;
LOADFILE fpLoadFile = NULL;

//wchar_t *working = NULL;

// Cached set of files that have an override file available
concurrency::concurrent_unordered_set<std::wstring> overrideSet;

// Cached set of files that don't have an override and should be loaded from archives
concurrency::concurrent_unordered_set<std::wstring> archiveSet;

void* tVirtualToArchivePath(DLString *path, UINT64 p2, UINT64 p3, DLString *p4, UINT64 p5, UINT64 p6)
{
	void *res = fpVirtualToArchivePath(path, p2, p3, p4, p5, p6);
	return (void*)ReplaceFileLoadPath((DLString*)res);
}

void* tFuckSekiro(SekiroString *path, UINT64 p2, UINT64 p3, DLString *p4, UINT64 p5, UINT64 p6)
{
	void *res = fpFuckSekiro(path, p2, p3, p4, p5, p6);
	if (res != NULL)
	{
		return ((char*)ReplaceFileLoadPath(&((SekiroString*)res)->string)) - 8;
	}
	return res;
}

wchar_t* gModDir = NULL;
bool gLoadUXMFiles = false;
bool gUseModOverride = false;
bool gCachePaths = false;



wchar_t* FindOverrideFile(const wchar_t *fullPath, const wchar_t *gamePath)
{
	int modDirLen = lstrlenW(gModDir);
	int fullPathLen = lstrlenW(fullPath);
	int gamePathLen = lstrlenW(gamePath);
	wchar_t *searchPath = (wchar_t*)malloc((modDirLen + fullPathLen + 2) * 2);
	if (searchPath == NULL)
	{
		return NULL;
	}
	lstrcpynW(searchPath, fullPath, fullPathLen + 1);
	lstrcpynW(searchPath + (fullPathLen - gamePathLen), gModDir, modDirLen + 1);
	lstrcpynW(searchPath + (fullPathLen - gamePathLen) + modDirLen, gamePath, gamePathLen + 1);

	if (GetFileAttributesW(searchPath) != INVALID_FILE_ATTRIBUTES)
	{
		return searchPath;
	}
	free(searchPath);
	return NULL;
}

wchar_t* DS3Path = L"DARK SOULS III\\Game\\";
wchar_t* SekiroPath = L"Sekiro\\";

HANDLE WINAPI tCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	// Check and see if a subdirectory is attempting to be accessed
	if (lpFileName != NULL)
	{
		wchar_t gp[MAX_PATH + 40];
		GetCurrentDirectoryW(MAX_PATH, gp);

		const wchar_t *gamepath = StrStrNIW(lpFileName, gp, MAX_PATH);
		const int dirSlashIdx = lstrlenW(gp) - 1;

		//wprintf(L"Path %s, GP %s, dirSlashIdx %d\r\n", lpFileName, gamepath, dirSlashIdx);
		 
		if (gamepath != NULL && dirSlashIdx > 0)
		{
			const wchar_t *gameDirSlash = wcsstr(&gamepath[dirSlashIdx], L"\\");
			if (gameDirSlash != NULL && gameDirSlash != lpFileName)
			{
				// We likely have a game directory path. Attempt to find override files
				wprintf(L"[FileHook] Intercepted game file path %s\r\n", lpFileName);
				wchar_t* orpath = FindOverrideFile(lpFileName, gameDirSlash);
				if (orpath != NULL)
				{
					wprintf(L"[FileHook] Overriding with %s\r\n", orpath);
					// Do 10 attempts at loading the override file since we know it exists
					for (int i = 0; i < 10; i++)
					{
						HANDLE res = fpCreateFileW(orpath, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
							dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
						if (res != INVALID_HANDLE_VALUE)
						{
							free(orpath);
							return res;
						}
						wprintf(L"Failed to load file %s (%d)th try\r\n", orpath, i);
					}
					free(orpath);
				}
			}
		}
	}
	return fpCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
		dwFlagsAndAttributes, hTemplateFile);
}

UINT64 tLoadFile(LPVOID arg1, UINT64 arg2)
{
	if (arg1 != NULL)
	{
		UINT8 *ptr = (UINT8*)arg1;
		UINT64 *length = (UINT64*)(ptr + 0x48);
		UINT64 *capacity = (UINT64*)(ptr + 0x50);
		if (*capacity < 8)
		{
			return fpLoadFile(arg1, arg2);
		}
		wchar_t **string = (wchar_t**)(ptr + 0x38);
		wchar_t *path = *string;
		//wprintf(L"[FileHook] Intercepted game file path %s\r\n", string);

		// Check and see if a subdirectory is attempting to be accessed
		if (path != NULL)
		{
			/*wchar_t *gp = NULL;
			int dirSlashIdx;
			if (GetGameType() == GAME_DARKSOULS_3)
			{
				gp = DS3Path;
				dirSlashIdx = 19;
			}
			else if (GetGameType() == GAME_SEKIRO)
			{
				gp = SekiroPath;
				dirSlashIdx = 6;
			}*/

			wchar_t gp[MAX_PATH+40];
			GetCurrentDirectoryW(MAX_PATH, gp);

			const wchar_t *gamepath = StrStrNIW(path, gp, MAX_PATH);
			const int dirSlashIdx = lstrlenW(gp) - 1;

			wprintf(L"Path %s, GP %s, dirSlashIdx %d\r\n", path, gamepath, dirSlashIdx);

			if (gamepath != NULL && dirSlashIdx > 0)
			{

				const wchar_t *gameDirSlash = StrStrNIW(&gamepath[dirSlashIdx], L"\\", MAX_PATH);
				if (gameDirSlash != NULL && gameDirSlash != path)
				{
					// We likely have a game directory path. Attempt to find override files
					if (gDebugLog)
					{
						wprintf(L"[FileHook] Intercepted game file path %s\r\n", path);
					}
					wchar_t* orpath = FindOverrideFile(path, gameDirSlash);
					if (orpath != NULL)
					{
						UINT64 oldLength = *length;
						UINT64 oldCapacity = *capacity;
						wchar_t* oldString = path;

						UINT64 newlen = lstrlenW(orpath) + 1;
						*length = newlen;
						*capacity = newlen;
						*string = orpath;

						if (gDebugLog)
						{
							wprintf(L"[FileHook] Overriding with %s\r\n", orpath);
						}
						UINT64 res = fpLoadFile(arg1, arg2);
						free(orpath);

						*length = oldLength;
						*capacity = oldCapacity;
						*string = oldString;
						return res;
					}
				}
			}
		}
	}
	return fpLoadFile(arg1, arg2);
}

DWORD64 ReplaceFileLoadPath(DLString *path)
{
	// Patch to load loose files
	wchar_t working[MAX_PATH];
	if (path->capacity > 7 && path->length >= 6)
	{
		if (path->string[0] == L'd' && path->string[1] == L'a' && path->string[2] == L't' && path->string[3] == L'a')
		{
			if (gDebugLog)
			{
				//wprintf(L"[ArchiveHook] Intercepted archive path %s\r\n", path->string);
			}

			// See if this is already cached
			bool isOverridden = gCachePaths ? (overrideSet.find(path->string) != overrideSet.end()) : false;
			bool isArchived = gCachePaths ? (archiveSet.find(path->string) != archiveSet.end()) : false;
			bool overr = isOverridden;

			// Not cached
			if (!isOverridden && !isArchived)
			{
				//wchar_t *working = (wchar_t*)malloc(1024);
				GetCurrentDirectoryW(MAX_PATH, working);
				if (gUseModOverride && !gLoadUXMFiles)
				{
					lstrcpynW(working + lstrlenW(working), gModDir, lstrlenW(gModDir) + 1);
				}
				lstrcpynW(working + lstrlenW(working), &path->string[6], path->length - 5);
				for (int i = 0; i < lstrlenW(working); i++)
				{
					if (working[i] == L'/')
					{
						working[i] = L'\\';
					}
				}
				if (gDebugLog)
				{
					wprintf(L"[ArchiveHook] Looking for override file %s\r\n", working);
				}
				if (GetFileAttributesW(working) != INVALID_FILE_ATTRIBUTES)
				{
					overr = true;
					if (gCachePaths)
						overrideSet.insert(path->string);
					if (gDebugLog)
					{
						wprintf(L"[ArchiveHook] Adding override for %s\r\n", working);
					}
				}
				else
				{
					if (gCachePaths)
						archiveSet.insert(path->string);
				}
				//free(working);
			}

			if (overr)
			{
				path->string[0] = L'.';
				path->string[1] = L'/';
				path->string[2] = L'/';
				path->string[3] = L'/';
				path->string[4] = L'/';
				path->string[5] = L'/';
				if (gDebugLog)
				{
					wprintf(L"[ArchiveHook] Path is now %s\r\n", path->string);
				}
			}
		}

		if (path->length >= 10)
		{
			if (path->string[0] == L'g' && path->string[1] == L'a' && path->string[2] == L'm' && path->string[3] == L'e' && path->string[4] == L'_')
			{
				wprintf(L"[ArchiveHook] Intercepted archive path %s\r\n", path->string);

				// See if this is already cached
				bool isOverridden = gCachePaths ? (overrideSet.find(path->string) != overrideSet.end()) : false;
				bool isArchived = gCachePaths ? (archiveSet.find(path->string) != archiveSet.end()) : false;
				bool overr = isOverridden;

				// Not cached
				if (!isOverridden && !isArchived)
				{
					//wchar_t *working = (wchar_t*)malloc(1024);
					GetCurrentDirectoryW(MAX_PATH, working);
					if (gUseModOverride && !gLoadUXMFiles)
					{
						lstrcpynW(working + lstrlenW(working), gModDir, lstrlenW(gModDir) + 1);
					}
					lstrcpynW(working + lstrlenW(working), &path->string[6], path->length - 5);
					for (int i = 0; i < lstrlenW(working); i++)
					{
						if (working[i] == L'/')
						{
							working[i] = L'\\';
						}
					}
					wprintf(L"[ArchiveHook] Looking for override file %s\r\n", working);
					if (GetFileAttributesW(working) != INVALID_FILE_ATTRIBUTES)
					{
						overr = true;
						if (gCachePaths)
							overrideSet.insert(path->string);
						wprintf(L"[ArchiveHook] Adding override for %s\r\n", working);
					}
					else
					{
						if (gCachePaths)
							archiveSet.insert(path->string);
					}
					//free(working);
				}

				if (overr)
				{
					path->string[0] = L'.';
					path->string[1] = L'/';
					path->string[2] = L'/';
					path->string[3] = L'/';
					path->string[4] = L'/';
					path->string[5] = L'/';
					path->string[6] = L'/';
					path->string[7] = L'/';
					path->string[8] = L'/';
					path->string[9] = L'/';
				}
			}
		}
	}

	return (DWORD64)path;
}

LPVOID GetArchiveFunctionAddress()
{
	DSGame game = GetGameType();
	if (game == GAME_DARKSOULS_3)
	{
		return (LPVOID)0x14007d5e0;
	}
	else if (game == GAME_SEKIRO)
	{
		//return (LPVOID)0x1401c5d80;
		//return (LPVOID)0x1401c5d80;
		unsigned short scanBytes[14] = {0x40, 0x55, 0x56, 0x41, 0x54, 0x41, 0x55, 0x48, 0x83, 0xec, 0x28, 0x4d, 0x8b, 0xe0};
		return AOBScanner::GetSingleton()->Scan(scanBytes, 14);
	}
	return NULL;
}

LPVOID GetLoadFileFunctionAddress()
{
	DSGame game = GetGameType();
	if (game == GAME_SEKIRO)
	{
		unsigned short scanBytes[14] = {0x40, 0x53, 0x56, 0x57, 0x41, 0x54, 0x48, 0x83, 0xec, 0x68, 0x8b, 0xfa, 0xc7, 0x84};
		return AOBScanner::GetSingleton()->Scan(scanBytes, 14);
	}
	return NULL;
}

// Mod loader hook by katalash
BOOL HookModLoader(bool loadUXMFiles, bool useModOverride, bool cachePaths, wchar_t *modOverrideDirectory)
{
	if (loadUXMFiles || useModOverride)
	{
		wprintf(L"[ModEngine] Hooking archive loader functions\r\n");
		// Hook the game archive function
		LPVOID hookAddress = GetArchiveFunctionAddress();
		if (hookAddress == NULL)
		{
			wprintf(L"[ModEngine] AOB Scan for archive function failed\r\n");
			return false;
		}
		else
		{
			wprintf(L"[ModEngine] AOB scan hooking archive function at %#p\r\n", hookAddress);
		}
		if (GetGameType() == GAME_SEKIRO)
		{
			if (MH_CreateHook(hookAddress, &tFuckSekiro, reinterpret_cast<LPVOID*>(&fpFuckSekiro)) != MH_OK)
				return false;

			if (MH_EnableHook(hookAddress) != MH_OK)
				return false;
		}
		else
		{
			if (MH_CreateHook(hookAddress, &tVirtualToArchivePath, reinterpret_cast<LPVOID*>(&fpVirtualToArchivePath)) != MH_OK)
				return false;

			if (MH_EnableHook(hookAddress) != MH_OK)
				return false;
		}
	}

	// Hook the windows file create/open api
	if (useModOverride && (modOverrideDirectory != NULL))
	{
		if (0)//GetGameType() == GAME_SEKIRO)
		{
			wprintf(L"[ModEngine] Hooking file loading functions\r\n");
			// Hook the game archive function
			LPVOID hookAddress = GetLoadFileFunctionAddress();
			if (hookAddress == NULL)
			{
				wprintf(L"[ModEngine] AOB Scan for file loading function failed\r\n");
				return false;
			}
			else
			{
				wprintf(L"[ModEngine] AOB scan file loading function at %#p\r\n", hookAddress);
			}
			if (MH_CreateHook(hookAddress, &tLoadFile, reinterpret_cast<LPVOID*>(&fpLoadFile)) != MH_OK)
				return false;

			if (MH_EnableHook(hookAddress) != MH_OK)
				return false;
		}
		else
		{
			wprintf(L"[ModEngine] Hooking CreateFileW\r\n");
			if (MH_CreateHookApi(L"kernel32", "CreateFileW", &tCreateFileW, reinterpret_cast<LPVOID*>(&fpCreateFileW)) != MH_OK)
				return false;

			if (MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleW(L"kernel32"), "CreateFileW")) != MH_OK)
				return false;
		}
	}

	gModDir = modOverrideDirectory;
	gLoadUXMFiles = loadUXMFiles;
	gUseModOverride = useModOverride && (modOverrideDirectory != NULL);
	gCachePaths = cachePaths;

	return true;
}