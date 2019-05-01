#pragma once
#include "MinHook/include/MinHook.h"
#include <list>

class AOBScanner
{
private:
	std::list<MEMORY_BASIC_INFORMATION> mMemoryList;
	LPVOID mBaseOffset;

	static AOBScanner *mSingleton;
public:
	AOBScanner();
	~AOBScanner();

	static AOBScanner* GetSingleton();
	void* Scan(unsigned short aob[], int numberOfBytes);
	void FindAndReplace(unsigned short aob[], unsigned char replace[], int numberOfBytes);
};

