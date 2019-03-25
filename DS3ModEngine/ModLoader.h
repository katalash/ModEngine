#pragma once

//#pragma once
#include "MinHook\include\MinHook.h"

BOOL HookModLoader(bool loadUXMFiles, bool useModOverride, bool cachePaths, wchar_t *modOverrideDirectory);

typedef struct
{
	wchar_t *string;
	void *unk;
	UINT64 length;
	UINT64 capacity;
} DLString;

typedef struct
{
	void *breaking;
	DLString string;
} SekiroString;

extern "C" DWORD64 __stdcall VirtualToArchivePathEpilogueHook;
extern "C" DWORD64 ReplaceFileLoadPath(DLString *path);