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

	return true;
}