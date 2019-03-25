#include "GameplayPatcher.h"
#include "Game.h"
#include "AOBScanner.h"
#include <stdio.h>

BOOL ApplyBonfireSacrificePatch()
{
	BYTE sacrificePatch[6] = {0xC6, 0x41, 0x10, 0x01, 0x90, 0x90};

	DWORD oldProtect;

	if (!VirtualProtect((LPVOID)0x1409AD0DE, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;
	memcpy((LPVOID)0x1409AD0DE, &sacrificePatch[0], 6);
	VirtualProtect((LPVOID)0x1409AD0DE, 6, oldProtect, &oldProtect);
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
	return true;
}