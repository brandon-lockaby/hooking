#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "windows.h"
#include "hook.h"
#include "console.h"
#pragma comment(lib, "user32.lib")
using namespace std;
#include "condition.h" // todo: remove this

HMODULE gHModule;

vector<string> strsplit(string input, string separator)
{
	int size = input.length();
	char* buffer = new char[size + 1];
	strncpy_s(buffer, size + 1, input.c_str(), size + 1);
	buffer[size] = '\0';
	vector<string> lines;
	char* context = NULL;
	char* result = strtok_s(buffer, separator.c_str(), &context);
	while(result)
	{
		lines.push_back(result);
		result = strtok_s(NULL, separator.c_str(), &context);
	}
	delete[] buffer;
	return lines;
}

LPVOID findApi(string modname, string procname)
{
	LPVOID address = 0;
	char* mod = new char[modname.length() + 1];
	strcpy_s(mod, modname.length() + 1, modname.c_str());
	char* proc = new char[procname.length() + 1];
	strcpy_s(proc, procname.length() + 1, procname.c_str());
	
	HMODULE hmod = GetModuleHandleA(mod);
	if(!hmod)
		log("Couldn't get handle to module " + string(mod));
	else
	{
		address = (LPVOID)GetProcAddress(hmod, proc);
		CloseHandle(hmod);
	}
	if(!address)
		log("Couldn't find " + modname + " " + procname);

	delete[] mod;
	delete[] proc;
	return address;
}

DWORD call(LPVOID address, int argc, DWORD* argv)
{
	DWORD retval;
	DWORD scratch;
	argv += (argc - 1);
	for(int i = 0; i < argc; i++)
	{
		scratch = *argv;
		__asm push scratch
		argv--;
	}
	__asm
	{
		mov eax, address
		call eax
		mov retval, eax
	}
	return retval;
}

DWORD WINAPI inputThread(LPVOID param)
{
	title();
	help();
	ostringstream oss;
	oss << "Business thread: " << (void*)GetCurrentThreadId();
	log(oss.str());

	// debug
	ConditionContext context;
	Condition* cond = new Condition(Condition::Imm, (void*)20, 0, Condition::Imm, (void*)20, 0, Condition::Equal);
	oss.str("");
	oss << "20 == 20: " << cond->evaluate(context);
	log(oss.str());
	cond = new Condition(Condition::Imm, (void*)20, 0, Condition::Imm, (void*)19, 0, Condition::Equal);
	oss.str("");
	oss << "20 == 19: " << cond->evaluate(context);
	log(oss.str());

	// fix the console window
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
		ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT
		| ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE);
	HWND window = GetConsoleWindow();
	RemoveMenu(GetSystemMenu(window, 0), SC_CLOSE, MF_BYCOMMAND);
	char* title = new char[1024];
	if(GetConsoleTitle(title, 1024) < 1012)
	{
		strcat_s(title, 1024, " - apilog");
		SetConsoleTitleA(title);
	}
	delete[] title;
	COORD coord;
	coord.X = 80;
	coord.Y = 9999;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coord);
	SMALL_RECT rect;
	rect.Left = 0;
	rect.Top = 0;
	rect.Right = 79;
	rect.Bottom = 39;
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

	while(1)
	{
		string input = getInput();
		vector<string> words = strsplit(input, " ");
		if(words.size() >= 1)
		{
			if(words[0] == "hooks")
			{
				if(gHooks.size() == 0)
				{
					log("None");
				}
				else for(vector<Hook>::iterator iter = gHooks.begin(); iter != gHooks.end(); ++iter)
				{
					oss.str("");
					oss << "#" << iter->mId << ": " << (void*)(iter->mAddress) << " " << iter->mName;
					log(oss.str());
				}
				continue;
			}
			else if(words[0] == "hook" || words[0] == "log")
			{
				if(words.size() >= 4 && words[1] == "api")
				{
					string mod = words[2];
					string proc = words[3];
					LPVOID address = findApi(mod, proc);
					if(address)
					{
						string name = mod + " " + proc;
						if(hook(address, name))
						{
							oss.str("");
							oss << "Hook installed: " + name +  " (" << (void*)address << ") -> " << getHookFunction();
							log(oss.str());
						}
					}
					continue;
				}
				else if(words.size() >= 2)
				{
					istringstream iss(words[1]);
					DWORD address;
					iss >> hex >> address;
					if(address)
					{
						if(hook((LPVOID)address, words[1]))
						{
							oss.str("");
							oss << "Hook installed: " << (void*)address << " -> " << getHookFunction();
							log(oss.str());
						}
						continue;
					}
				}
			}
			else if(words.size() == 2 && words[0] == "unhook")
			{
				istringstream iss(words[1]);
				DWORD address;
				iss >> hex >> address;
				if(address)
				{
					if(unhook((LPVOID)address))
						log("Hook uninstalled");
					else
						log("Fail");
					continue;
				}
			}
			else if(words.size() >= 2 && words[0] == "call")
			{
				LPVOID address = 0;
				int argi = -1;
				if(words[1] == "api" && words.size() >= 4)
				{
					argi = 4;
					address = findApi(words[2], words[3]);
				}
				else
				{
					argi = 2;
					istringstream iss(words[1]);
					iss >> hex >> address;
				}
				if(!address)
				{
					log("Fail, no address");
					continue;
				}
				int argc = words.size() - argi;
				DWORD* argv = NULL;
				if(argc)
				{
					argv = new DWORD[argc];
					int x = 0;
					for(int i = argi; i < words.size(); i++)
					{
						istringstream iss(words[i]);
						iss >> hex >> argv[x];
						x++;
					}
				}
				DWORD retval = call(address, argc, argv);
				oss.str("");
				oss << "Your call returned: " << (void*)retval;
				log(oss.str());
				continue;
			}
			else if(words[0] == "q" || words[0] == "quit" || words[0] == "exit")
			{
				unhookAll();
				FreeConsole();
				__asm
				{
					push 0
					push 0xC0DEC0DE
					push gHModule
					push ExitThread
					push FreeLibrary
					ret
				}
			}
			else if(words[0] == "cls")
			{
				system("cls");
				continue;
			}
		}
		log("What?");
	}
	return 0;
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	DWORD scratch;
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		gHModule = hinstDLL;

		AllocConsole();
		ostringstream oss;
		oss << "DLL_PROCESS_ATTACH. Module address: " << (void*)hinstDLL << " Thread ID: " << (void*)GetCurrentThreadId();
		log(oss.str());
		oss.str("");
		oss << "hook function: " << getHookFunction();
		log(oss.str());
		CreateThread(0, 0, inputThread, 0, 0, 0);
	}
	else if(fdwReason == DLL_PROCESS_DETACH)
	{
		ostringstream oss;
		oss << "DLL_PROCESS_DETACH. Thread ID: " << (void*)GetCurrentThreadId();
		log(oss.str());
	}
	else if(fdwReason == DLL_THREAD_ATTACH)
	{
		ostringstream oss;
		oss << "DLL_THREAD_ATTACH. Thread ID: " << (void*)GetCurrentThreadId();
		log(oss.str());
	}
    else if(fdwReason == DLL_THREAD_DETACH)
	{
		ostringstream oss;
		oss << "DLL_THREAD_DETACH. Thread ID: " << (void*)GetCurrentThreadId();
		log(oss.str());
	}
    return TRUE;
}
