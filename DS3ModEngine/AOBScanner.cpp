#include "AOBScanner.h"
#include <windows.h>
#include <Psapi.h>

AOBScanner* AOBScanner::mSingleton = nullptr;

AOBScanner::AOBScanner()
{
	// Get process regions
	HANDLE procHandle = GetCurrentProcess();

	unsigned char *ptr = NULL;
	MEMORY_BASIC_INFORMATION info;
	for (ptr = NULL; VirtualQueryEx(procHandle, ptr, &info, sizeof(info)) == sizeof(info); ptr += info.RegionSize)
	{
		if (info.State == MEM_COMMIT)
		{
			info.BaseAddress = ptr;
			mMemoryList.push_back(info);
		}
	}

	MODULEINFO moduleInfo;
	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS ntHeader;
	IMAGE_FILE_HEADER header;
	if (GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &moduleInfo, sizeof(moduleInfo)))
	{
		mBaseOffset = (LPVOID)moduleInfo.lpBaseOfDll;
	}

	wprintf(L"[ModEngine] Detected base module offset at 0x%#p\r\n", mBaseOffset);
	wprintf(L"[ModEngine] AOB Scanner Initialized\r\n");
}


AOBScanner::~AOBScanner()
{
}

AOBScanner* AOBScanner::GetSingleton()
{
	if (mSingleton == nullptr)
	{
		mSingleton = new AOBScanner();
	}
	return mSingleton;
}

void* AOBScanner::Scan(unsigned short aob[], int numberOfBytes)
{
	DWORD oldProtect;
	for (auto meminfo : mMemoryList)
	{
		// Only scan exe memory
		if (((UINT64)meminfo.BaseAddress & 0xFFFFFFFF00000000) != ((UINT64)mBaseOffset & 0xFFFFFFFF00000000))
		{
			continue;
		}
		if (!VirtualProtect(meminfo.BaseAddress, meminfo.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			continue;
		}

		for (auto addr = (unsigned char*)meminfo.BaseAddress;
			addr < (unsigned char*)meminfo.BaseAddress + meminfo.RegionSize - numberOfBytes; addr++)
		{
			bool found = true;
			for (int j = 0; j < numberOfBytes; j++)
			{
				if (aob[j] < 256 && addr[j] != (unsigned char)(aob[j] & 0xFF))
				{
					found = false;
					break;
				}
			}
			if (found)
			{
				if (!VirtualProtect(meminfo.BaseAddress, meminfo.RegionSize, oldProtect, &oldProtect))
					return NULL;
				return addr;
			}
		}

		if (!VirtualProtect(meminfo.BaseAddress, meminfo.RegionSize, oldProtect, &oldProtect))
			return NULL;
	}
	return NULL;
}

void AOBScanner::FindAndReplace(unsigned short aob[], unsigned char replace[], int numberOfBytes)
{
	DWORD oldProtect;
	for (auto meminfo : mMemoryList)
	{
		// Only scan exe memory
		if (((UINT64)meminfo.BaseAddress & 0xFFFFFFFFF0000000) != (UINT64)mBaseOffset)
		{
			continue;
		}
		if (!VirtualProtect(meminfo.BaseAddress, meminfo.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			continue;
		}

		for (auto addr = (unsigned char*)meminfo.BaseAddress;
			addr < (unsigned char*)meminfo.BaseAddress + meminfo.RegionSize - numberOfBytes; addr++)
		{
			bool found = true;
			for (int j = 0; j < numberOfBytes; j++)
			{
				if (aob[j] < 256 && addr[j] != (unsigned char)(aob[j] & 0xFF))
				{
					found = false;
					break;
				}
			}
			if (found)
			{
				wprintf(L"[ModEngine] AOB scan found %#p\r\n", addr);
				memcpy(addr, replace, numberOfBytes);
			}
		}

		VirtualProtect(meminfo.BaseAddress, meminfo.RegionSize, oldProtect, &oldProtect);
	}
}