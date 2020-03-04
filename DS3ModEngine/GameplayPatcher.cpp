#include "GameplayPatcher.h"
#include "Game.h"
#include "AOBScanner.h"
#include <stdio.h>

BOOL ApplyBonfireSacrificePatch()
{
	BYTE sacrificePatch[6] = {0xC6, 0x41, 0x10, 0x01, 0x90, 0x90};

	DWORD oldProtect;

	// Supposed to be at 0x1409AD0DE
	unsigned short scanBytes[14] = { 0x66, 0xC7, 0x41, 0x10, 0x00, 0x00, 0x48, 0x8B, 0x88, 0xC8, 0x11, 0x00, 0x00, 0x48 };
	LPVOID addr = AOBScanner::GetSingleton()->Scan(scanBytes, 14);
	if (!VirtualProtect(addr, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;
	memcpy((LPVOID)0x1409AD0DE, &sacrificePatch[0], 6);
	VirtualProtect(addr, 6, oldProtect, &oldProtect);
	return true;
}

BOOL ApplyGameplayPatches()
{
	bool sacrificePatch = (GetPrivateProfileIntW(L"gameplay", L"restoreBonfireSacrifice", 0, L".\\modengine.ini") == 1);

	if (sacrificePatch)
	{
		wprintf(L"[ModEngine] Patching in bonfire sacrifice mechanic\r\n");
		if (!ApplyBonfireSacrificePatch())
		{
			return false;
		}
	}
	return true;
}

BOOL ApplyNoLogoPatch()
{
	DWORD oldProtect;
	if (GetGameType() == GAME_SEKIRO)
	{
		unsigned short aob[30] = {0x74, 0x30, 0x48, 0x8d, 0x54, 0x24, 0x30, 0x48, 0x8b, 0xcd,
		                          0xe8, 0x100,0x100,0x100,0x100,0x90, 0xbb, 0x01, 0x00, 0x00,
		                          0x00, 0x89, 0x5c, 0x24, 0x20, 0x44, 0x0f, 0xb6, 0x4e, 0x04};
		LPVOID address = AOBScanner::GetSingleton()->Scan(aob, 30);
		if (address != NULL)
		{
			wprintf(L"[ModEngine] Applying No logo patch to %#p\r\n", address);
			if (!VirtualProtect((LPVOID)address, 1, PAGE_READWRITE, &oldProtect))
				return false;
			*(char*)address += 1;
			VirtualProtect((LPVOID)address, 1, oldProtect, &oldProtect);
		}
		else
		{
			wprintf(L"[ModEngine] AOB scan failed to find no logo patch\r\n");
		}
	}
	return true;
}

typedef void* (*FXRCONSTRUCTOR)(LPVOID, LPVOID, wchar_t*, LPVOID, UINT64);
FXRCONSTRUCTOR fpFXRConstructor = NULL;
void* tFXRConstructor(LPVOID p1, LPVOID p2, wchar_t* fxrname, LPVOID p4, UINT64 p5)
{
	wprintf(L"[FXR] Engine loading FXR %s (object %#p)\r\n", fxrname, p1);
	return fpFXRConstructor(p1, p2, fxrname, p4, p5);
}

typedef void* (*FXR1)(LPVOID, LPVOID);
FXR1 fpFXR1 = NULL;
void* tFXR1(LPVOID p1, LPVOID p2)
{
	wprintf(L"[FXR] Loaded to object %#p\r\n", p2);
	return fpFXR1(p1, p2);
}

typedef void* (*MSBHITCONSTRUCTOR)(LPVOID, LPVOID, LPVOID, LPVOID, char);
MSBHITCONSTRUCTOR fpMSBHitConstructor = NULL;
void* tMSBHitConstructor(LPVOID p1, LPVOID p2, LPVOID p3, LPVOID p4, char p5)
{
	wprintf(L"[HIT] Engine loading MSB Hit (object %#p)\r\n", p1);
	LPVOID hit = fpMSBHitConstructor(p1, p2, p3, p4, p5);
	wprintf(L"Disp groups: %#d %#d %#d %#d %#d %#d %#d %#d\r\n", *((char*)hit + 0x6C), *((char*)hit + 0x70), *((char*)hit + 0x74), *((char*)hit + 0x78), *((char*)hit + 0x7C), *((char*)hit + 0x80), *((char*)hit + 0x84), *((char*)hit + 0x88));
	return hit;
}


typedef void* (*MEMORYALLOCATE)(UINT32, UINT32, LPVOID);
MEMORYALLOCATE fpMemoryAllocate = NULL;
void* tMemoryAllocate(UINT32 size, UINT32 unk, LPVOID allocator)
{
	LPVOID ret = fpMemoryAllocate(size, unk, allocator);
	if (ret == NULL)
	{
		wprintf(L"[Allocator] Allocation of size %d with allocator at %#p\r\n", size, allocator);
		wprintf(L"[Allocator] Allocation failed\r\n");
	}
	return ret;
}

BOOL gPatchedAllocatorLimits = false;

typedef HANDLE(WINAPI* VIRTUALALLOC)(LPVOID, SIZE_T, DWORD, DWORD);
VIRTUALALLOC fpVirtualAlloc = NULL;
LPVOID tVirtualAlloc(
	LPVOID lpAddress,
	SIZE_T dwSize,
	DWORD  flAllocationType,
	DWORD  flProtect
)
{
	if (!gPatchedAllocatorLimits)
	{
		ApplyDS3SekiroAllocatorLimitPatch();
		//onSteamInit();
		gPatchedAllocatorLimits = true;
	}
	return fpVirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}

BOOL ApplyAllocationTracer()
{
	if (GetGameType() == GAME_DARKSOULS_2_SOTFS)
	{
		if (MH_CreateHook((LPVOID)0x14082bdc0, &tMemoryAllocate, reinterpret_cast<LPVOID*>(&fpMemoryAllocate)) != MH_OK)
			return false;

		if (MH_EnableHook((LPVOID)0x14082bdc0) != MH_OK)
			return false;
	}
}

BOOL ApplyMiscPatches()
{
	bool noLogo = (GetPrivateProfileIntW(L"misc", L"skipLogos", 1, L".\\modengine.ini") == 1);

	if (noLogo)
	{
		if (!ApplyNoLogoPatch())
		{
			return false;
		}
	}

	if (GetGameType() == GAME_DARKSOULS_3)
	//if (0)
	{
		/*if (MH_CreateHook((LPVOID)0x140e60320, &tFXRConstructor, reinterpret_cast<LPVOID*>(&fpFXRConstructor)) != MH_OK)
			return false;

		if (MH_EnableHook((LPVOID)0x140e60320) != MH_OK)
			return false;

		if (MH_CreateHook((LPVOID)0x141ac32f0, &tFXR1, reinterpret_cast<LPVOID*>(&fpFXR1)) != MH_OK)
			return false;

		if (MH_EnableHook((LPVOID)0x141ac32f0) != MH_OK)
			return false;*/
		if (MH_CreateHook((LPVOID)0x14084ea60, &tMSBHitConstructor, reinterpret_cast<LPVOID*>(&fpMSBHitConstructor)) != MH_OK)
			return false;

		if (MH_EnableHook((LPVOID)0x14084ea60) != MH_OK)
			return false;
	}

	//ApplyAllocationTracer();
	//ApplyFModHooks();
	ApplyDS3SekiroAllocatorLimitPatch();

	return true;
}

BOOL ApplyShadowMapResolutionPatches(int dirSize, int atlasSize, int pointSize, int spotSize)
{
	DWORD oldProtect;
	if (GetGameType() == GAME_DARKSOULS_2_SOTFS)
	{
		// 0x1404ac3ed
		unsigned short aobDir[27] =   { 0xc7, 0x44, 0x24, 0x28, 0x00, 0x08, 0x00, 0x00, 0x48,
									    0x89, 0x44, 0x24, 0x30, 0x48, 0x8b, 0x47, 0x40, 0xc7,
									    0x44, 0x24, 0x2C, 0x00, 0x08, 0x00, 0x00, 0x48, 0x89 };

		// 0x140b96e30
		unsigned short aobPoint[27] = { 0xC7, 0x01, 0x00, 0x02, 0x00, 0x00, 0xC7, 0x41, 0x04,
									    0x02, 0x00, 0x00, 0x00, 0xC7, 0x41, 0x08, 0x04, 0x00,
									    0x00, 0x00, 0xC7, 0x41, 0x0C, 0x06, 0x00, 0x00, 0x00};

		char *addressDir = (char*)AOBScanner::GetSingleton()->Scan(aobDir, 27);
		if (addressDir != NULL)
		{
			wprintf(L"[ModEngine] Patching directional shadow resolution of %d to %#p\r\n", dirSize, addressDir);
			if (!VirtualProtect((LPVOID)addressDir, 30, PAGE_READWRITE, &oldProtect))
				return false;
			// Patch hardcoded mov instructions with new resolution
			memcpy(&addressDir[4],  &dirSize, 4);
			memcpy(&addressDir[21], &dirSize, 4);
			VirtualProtect((LPVOID)addressDir, 30, oldProtect, &oldProtect);
		}
		else
		{
			wprintf(L"[ModEngine] AOB scan failed to find directional shadow resolution\r\n");
		}

		char* addressPoint = (char*)AOBScanner::GetSingleton()->Scan(aobPoint, 27);
		if (addressPoint != NULL)
		{
			wprintf(L"[ModEngine] Patching dynamic shadow resolution of (atlas: %d, point: %d, spot: %d) to %#p\r\n", atlasSize, pointSize, spotSize, addressPoint);
			if (!VirtualProtect((LPVOID)addressPoint, 30, PAGE_READWRITE, &oldProtect))
				return false;
			// Patch hardcoded mov instructions with new resolution
			memcpy(&addressPoint[2], &spotSize, 4);
			memcpy(&addressPoint[37], &pointSize, 4);
			memcpy(&addressPoint[72], &atlasSize, 4);
			VirtualProtect((LPVOID)addressPoint, 30, oldProtect, &oldProtect);
		}
		else
		{
			wprintf(L"[ModEngine] AOB scan failed to find dynamic shadow resolution\r\n");
		}
	}
}

typedef void* (*FMODMEMORYALLOCATE)(UINT32, UINT32, LPVOID);
FMODMEMORYALLOCATE fpFmodMemoryAllocate = NULL;
void* tFmodMemoryAllocate(UINT32 size, UINT32 unk, LPVOID allocator)
{
	LPVOID ret = fpFmodMemoryAllocate(size, unk, allocator);
	wprintf(L"[Allocator] FMOD Allocation of size %d with typet %d\r\n", size, unk);
	if (ret == NULL)
	{
		wprintf(L"[Allocator] FMOD Allocation failed\r\n");
		while (1) {}
	}
	return ret;
}

BOOL ApplyFModHooks()
{
	if (GetGameType() == GAME_DARKSOULS_3)
	{
		if (MH_CreateHook((LPVOID)0x141a4bfd0, &tFmodMemoryAllocate, reinterpret_cast<LPVOID*>(&fpFmodMemoryAllocate)) != MH_OK)
			return false;

		if (MH_EnableHook((LPVOID)0x141a4bfd0) != MH_OK)
			return false;
	}
}

BOOL ApplyDS3SekiroAllocatorLimitPatch()
{
	DWORD oldProtect;
	if (GetGameType() == GAME_DARKSOULS_3 || GetGameType() == GAME_SEKIRO)
	{
		// Find limit table
		unsigned short table[56] =  { 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00,
									  0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00,
									  0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
									  0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
									  0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
									  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
									  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };
		uint64_t* tablePtr = (uint64_t*)AOBScanner::GetSingleton()->Scan(table, 56);
		if (tablePtr != NULL)
		{
			wprintf(L"[ModEngine] Patching memory limit table at %#p\r\n", tablePtr);
			if (!VirtualProtect((LPVOID)tablePtr, 0x100, PAGE_READWRITE, &oldProtect))
				return false;
			// FMod allocator limit
			tablePtr[9] = tablePtr[9] * 3;
			tablePtr[10] = tablePtr[10] * 3;
			VirtualProtect((LPVOID)tablePtr, 0x100, oldProtect, &oldProtect);
		}
		else
		{
			wprintf(L"[ModEngine] Could not find allocation limit table\r\n");
			return false;
		}
		return true;
	}
}

BOOL ApplyAllocatorLimitPatchVA()
{
	wprintf(L"[ModEngine] Hooking VirtualAlloc\r\n");
	if (MH_CreateHookApi(L"kernel32", "VirtualAlloc", &tVirtualAlloc, reinterpret_cast<LPVOID*>(&fpVirtualAlloc)) != MH_OK)
		return false;

	if (MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleW(L"kernel32"), "VirtualAlloc")) != MH_OK)
		return false;

	return true;
}