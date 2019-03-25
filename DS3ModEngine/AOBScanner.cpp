#include "AOBScanner.h"

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
		if (((UINT64)meminfo.BaseAddress & 0xFF0000000) != 0x140000000)
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