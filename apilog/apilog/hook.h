//#ifndef HOOK_H
//#define HOOK_H

#include <vector>
#include "windows.h"
using namespace std;

#define PATCHSIZE 11

struct Hook
{
	char* mName;
	unsigned int mId;

	void* mAddress;

	unsigned int mPatchSize;
	unsigned char* mPatchBytes;
	unsigned char* mUnpatchBytes;

	int mFlags;

	enum
	{
		HF_LOG = 1
	};
};

extern vector<Hook> gHooks;
extern unsigned int gCallCount;

void* getHookFunction();

Hook* findHookByAddress(void* address);
vector<Hook>::iterator findHookIterator(Hook* hookp);

bool hook(void* address, string name);
bool unhook(void* address);
void unhookAll();

bool patch(void* address, unsigned char* data, int len);

//#endif
