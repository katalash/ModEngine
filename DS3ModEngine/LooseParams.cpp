#include "LooseParams.h"
#include <stdio.h>

// Patch developed by Pav to load loose params instead of data0
BOOL LooseParamsPatch(bool saveLocationPatch, bool looseParamPatch)
{
	// ASM Patches
	BYTE branchPatch1[2] = { 0xEB, 0x68 };
	BYTE branchPatch2[6] = { 0x0F, 0x84, 0xc5, 0x00, 0x00, 0x00 };
	BYTE branchPatch3[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	DWORD oldProtect;

	if (saveLocationPatch)
	{
		wprintf(L"[ModEngine] Patching save file location\r\n");
		// Dumb save file location patch
		if (!VirtualProtect((LPVOID)0x143a72e9e, 1, PAGE_READWRITE, &oldProtect))
			return false;
		// Invent the .sl3 format
		*(char*)0x143a72e9e += 1;
		VirtualProtect((LPVOID)0x143a72e9e, 1, oldProtect, &oldProtect);
	}

	if (looseParamPatch)
	{
		wprintf(L"[ModEngine] Applying loose param patch\r\n");
		// Apply patch 1
		if (!VirtualProtect((LPVOID)0x140E15CF5, 2, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;
		memcpy((LPVOID)0x140E15CF5, &branchPatch1[0], 2);
		VirtualProtect((LPVOID)0x140E15CF5, 2, oldProtect, &oldProtect);

		// Apply patch 2
		if (!VirtualProtect((LPVOID)0x1408F6E59, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;
		memcpy((LPVOID)0x1408F6E59, &branchPatch2[0], 6);
		VirtualProtect((LPVOID)0x1408F6E59, 6, oldProtect, &oldProtect);

		// Apply patch 3
		if (!VirtualProtect((LPVOID)0x140E93D33, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;
		memcpy((LPVOID)0x140E93D33, &branchPatch3[0], 6);
		VirtualProtect((LPVOID)0x140E93D33, 6, oldProtect, &oldProtect);
	}

	return true;
}