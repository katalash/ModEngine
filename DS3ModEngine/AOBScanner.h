#pragma once
#include "MinHook/include/MinHook.h"
#include <list>

class AOBScanner
{
private:
	std::list<MEMORY_BASIC_INFORMATION> mMemoryList;

	static AOBScanner *mSingleton;
public:
	AOBScanner();
	~AOBScanner();

	static AOBScanner* GetSingleton();
	void* Scan(unsigned short aob[], int numberOfBytes);
};

