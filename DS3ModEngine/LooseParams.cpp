#include "LooseParams.h"
#include "Game.h"
#include "AOBScanner.h"
#include <stdio.h>

// Patch developed by Pav to load loose params instead of data0
BOOL LooseParamsPatch(bool saveLocationPatch, bool looseParamPatch)
{
	// ASM Patches
	BYTE branchPatch1[2] = { 0xEB, 0x68 };
	BYTE branchPatch2[6] = { 0x0F, 0x84, 0xc5, 0x00, 0x00, 0x00 };
	BYTE branchPatch3[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	DWORD oldProtect;

	if (saveLocationPatch && GetGameType() != GAME_SEKIRO)
	{
		wprintf(L"[ModEngine] Patching save file location\r\n");
		// Dumb save file location patch
		//if (!VirtualProtect((LPVOID)0x143a72e9e, 1, PAGE_READWRITE, &oldProtect))
		//	return false;
		// Invent the .sl3 format
		//*(char*)0x143a72e9e += 1;
		//VirtualProtect((LPVOID)0x143a72e9e, 1, oldProtect, &oldProtect);
		//unsigned short scanBytes[3] = { 's', 'l', '2' };
		unsigned short scanBytes2[5] = { 's', '\0', 'l', '\0', '2' };
		//unsigned char replaceBytes[3] = { 's', 'l', '3' };
		unsigned char replaceBytes2[5] = { 's', '\0', 'l', '\0', '3' };
		//AOBScanner::GetSingleton()->FindAndReplace(scanBytes, replaceBytes, 3);
		AOBScanner::GetSingleton()->FindAndReplace(scanBytes2, replaceBytes2, 5);
	}

	if (looseParamPatch && GetGameType() == GAME_DARKSOULS_3)
	{
		wprintf(L"[ModEngine] Applying loose param patch\r\n");
		// Apply patch 1
		// Supposed to be located at 0x140e15cf5
		unsigned short patch1Scan[14] = { 0x74, 0x68, 0x48, 0x8b, 0xcf, 0x48, 0x89, 0x5c, 0x24, 0x30, 0xe8, 0x1c, 0x6d, 0x08 };
		LPVOID patch1addr = AOBScanner::GetSingleton()->Scan(patch1Scan, 14);
		if (!VirtualProtect(patch1addr, 2, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;
		memcpy(patch1addr, &branchPatch1[0], 2);
		VirtualProtect(patch1addr, 2, oldProtect, &oldProtect);

		// Apply patch 2
		// Supposed to be located at 0x1408F6E59
		unsigned short patch2Scan[14] = { 0x0F, 0x85, 0xC5, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x4C, 0x24, 0x28, 0xE8, 0x17, 0xF4 };
		LPVOID patch2addr = AOBScanner::GetSingleton()->Scan(patch2Scan, 14);
		if (!VirtualProtect(patch2addr, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;
		memcpy(patch2addr, &branchPatch2[0], 6);
		VirtualProtect(patch2addr, 6, oldProtect, &oldProtect);

		// Apply patch 3
		// Supposed to be located at 0x140E93D33
		unsigned short patch3Scan[14] = { 0xE8, 0x78, 0x08, 0xF8, 0xFF, 0x90, 0xE9, 0x0E, 0xE9, 0x08, 0x05, 0x53, 0xE9, 0xEF };
		LPVOID patch3addr = AOBScanner::GetSingleton()->Scan(patch3Scan, 14);
		if (!VirtualProtect(patch3addr, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;
		memcpy(patch3addr, &branchPatch3[0], 6);
		VirtualProtect(patch3addr, 6, oldProtect, &oldProtect);
	}

	return true;
}