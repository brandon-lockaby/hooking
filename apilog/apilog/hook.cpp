#include "hook.h"
#include "console.h"

// Global hook collection
vector<Hook> gHooks;

// Keeps track of how many calls have been intercepted
unsigned int gCallCount = 0;

// Global variables used in hook function
// because I don't want the trouble of them being on the stack
Hook*   gHookp;
LPVOID  gHookedAddress;
LPVOID  gReturnAddress;
DWORD*  gESP;
DWORD   gReturnValue;
#define GSTR_SIZE 512
char    gStr[GSTR_SIZE];

// patched functions call this
static __declspec(naked) void hookFunction(void)
{
	// patched function left its address on the stack for us
	// pop that, then take a copy of the return address and esp
	__asm
	{
		pop gHookedAddress
		mov gESP, esp
		pop gReturnAddress
		push gReturnAddress
	}

	// find the info about this hook
	gHookp = findHookByAddress(gHookedAddress);
	if(!gHookp)
	{
		logline();
		sprintf_s(gStr, GSTR_SIZE, "UNEXPECTED CALL TO HOOK FUNCTION; Thread: %08X", GetCurrentThreadId());
		logc(gStr);
		// without knowledge of how many arguments are on the stack,
		// it's going to crash. restore stack, dump, and break
		__asm push gHookedAddress
		gESP--;
		dump0(gESP, 16);
		system("pause");
		__asm { int 3 } __asm { int 3 } __asm { int 3 } __asm { int 3 } 
		__asm { int 3 } __asm { int 3 } __asm { int 3 } __asm { int 3 } 
		__asm { int 3 } __asm { int 3 } __asm { int 3 } __asm { int 3 } 
		__asm { int 3 } __asm { int 3 } __asm { int 3 } __asm { int 3 } 
	}

	// bookkeeping
	++gCallCount;

	// log function name and stack
	if(gHookp->mFlags & Hook::HF_LOG)
	{
		logline();
		sprintf_s(gStr, GSTR_SIZE, "#%d: CALL %s (%08X); Thread: %08X", gCallCount, gHookp->mName, gHookp->mAddress, GetCurrentThreadId());
		logc(gStr);
		dump0(gESP, 16);
	}

	// remove return address
	__asm add esp, 4

	//

	// unpatch
	patch(gHookp->mAddress, gHookp->mUnpatchBytes, PATCHSIZE);

	// jump to original function
	__asm
	{
		push retlabel
		push gHookedAddress
		ret
	}

retlabel:

	// store return value
	__asm mov gReturnValue, eax

	// show return value
	sprintf_s(gStr, GSTR_SIZE, "#%d Returned: %08X", gCallCount, gReturnValue);
	logc(gStr);

	// re-patch
	patch(gHookp->mAddress, gHookp->mPatchBytes, PATCHSIZE);

	// return to caller
	__asm
	{
		mov eax, gReturnValue
		push gReturnAddress
		ret
	}
}

void* getHookFunction()
{
	return (void*)hookFunction;
}

Hook* findHookByAddress(void* address)
{
	for(vector<Hook>::iterator iter = gHooks.begin(); iter != gHooks.end(); ++iter)
	{
		if((*iter).mAddress == address)
			return (Hook*)&(*iter);
	}
	return NULL;
}

vector<Hook>::iterator findHookIterator(Hook* hookp)
{
	for(vector<Hook>::iterator iter = gHooks.begin(); iter != gHooks.end(); ++iter)
	{
		if(hookp == &(*iter))
			return iter;
	}
	return gHooks.end();
}

int getHookCount()
{
	return gHooks.size();
}

bool hook(void* address, string name)
{
	// unprotect just in case we can't read it idk
	DWORD oldprotect;
	if(!VirtualProtect(address, PATCHSIZE, PAGE_EXECUTE_READWRITE, &oldprotect))
	{
		log("Couldn't unprotect memory");
		return false;
	}

	// create
	Hook hook;
	hook.mName = new char[name.length() + 1];
	strcpy_s(hook.mName, name.length() + 1, name.c_str());
	hook.mAddress = address;
	hook.mPatchSize = PATCHSIZE;
	hook.mUnpatchBytes = new unsigned char[hook.mPatchSize];
	memcpy_s(hook.mUnpatchBytes, hook.mPatchSize, hook.mAddress, hook.mPatchSize);
	unsigned char* code = new unsigned char[hook.mPatchSize];
	code[0] = 0x68; // push dword
	code[5] = 0x68; // push dword
	code[10] = 0xC3; // ret
	DWORD* x = (DWORD*)&(code[1]);
	*x = (DWORD)address;
	x = (DWORD*)&(code[6]);
	*x = (DWORD)getHookFunction();
	hook.mPatchBytes = code;
	hook.mFlags = Hook::HF_LOG;

	// patch
	if(!patch(hook.mAddress, hook.mPatchBytes, PATCHSIZE))
		return false;

	// reprotect
	VirtualProtect(hook.mAddress, hook.mPatchSize, oldprotect, &oldprotect);
	gHooks.push_back(hook);
	
	return true;
}

bool unhook(void* address)
{
	// unpatch
	DWORD oldprotect;
	if(!VirtualProtect(address, PATCHSIZE, PAGE_EXECUTE_READWRITE, &oldprotect))
	{
		log("Couldn't unprotect memory");
		return false;
	}
	Hook* hookp = findHookByAddress(address);
	if(!hookp)
	{
		log("Hook not found");
		return false;
	}
	if(!patch(hookp->mAddress, hookp->mUnpatchBytes, hookp->mPatchSize)) // todo: convert them all to use this size
	{
		log("Unpatch failed");
		return false;
	}
	// todo: reprotect?
	vector<Hook>::iterator iter = findHookIterator(hookp);
	if(iter != gHooks.end())
		gHooks.erase(iter);
	return true;
}

void unhookAll()
{
	for(vector<Hook>::iterator iter = gHooks.begin(); iter != gHooks.end(); )
	{
		unhook(iter->mAddress);
	}
	gHooks.clear();
}

bool patch(void* address, unsigned char* data, int len)
{
	DWORD oldprotect;
	if(!VirtualProtect(address, len, PAGE_EXECUTE_READWRITE, &oldprotect))
	{
		log("Couldn't unprotect memory");
		return false;
	}
	memcpy_s(address, len, data, len);
	if(!VirtualProtect(address, PATCHSIZE, oldprotect, &oldprotect))
	{
		log("Couldn't reprotect memory");
	}
	return true;
}
