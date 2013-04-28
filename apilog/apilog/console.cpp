#include "console.h"
#include <sstream>

void logvoid(char* name, DWORD val)
{
	ostringstream oss;
	oss << name << " " << (void*)val;
	log(oss.str());
}

void dump0(DWORD* address, int dwords)
{
	char scratch[128];
	char scratch2[128];
	char* ptr;
	char ch;
	MEMORY_BASIC_INFORMATION mem_info;
	for(int i = 0; i < dwords; i++)
	{
		sprintf_s(scratch, 128, "%08X", *address);
		if(!IsBadReadPtr((void*)(*address), 1))
		{
			strcat_s(scratch, 128, " ");
			ptr = (char*)(*address);
			// show protection
			if(!VirtualQuery(ptr, &mem_info, sizeof(MEMORY_BASIC_INFORMATION)))
			{
				strcat_s(scratch, 128, "----");
			}
			else
			{
				ch = mem_info.Protect & 0xFF;
				if(ch == 0x10)
					strcat_s(scratch, 128, "x---");
				else if(ch == 0x20)
					strcat_s(scratch, 128, "xr--");
				else if(ch == 0x40)
					strcat_s(scratch, 128, "xrw-");
				else if(ch == 0x80)
					strcat_s(scratch, 128, "xrwc");
				else if(ch == 0x01)
					strcat_s(scratch, 128, "----");
				else if(ch == 0x02)
					strcat_s(scratch, 128, "-r--");
				else if(ch == 0x04)
					strcat_s(scratch, 128, "-rw-");
				else if(ch == 0x08)
					strcat_s(scratch, 128, "-rwc");
				else
					strcat_s(scratch, 128, "????");
			}
			// show as characters
			strcat_s(scratch, 128, " ");
			for(int x = 0; x < 64; x++)
			{
				if(IsBadReadPtr(ptr, 1))
					strcpy_s(scratch2, 128, "?");
				else
				{
					if(ptr[0] < 0x20 || ptr[0] > 0x7F)
						strcpy_s(scratch2, 128, ".");
					else
						sprintf_s(scratch2, 128, "%c", *ptr);
				}
				ptr++;
				strcat_s(scratch, 128, scratch2);
			}
			
		}
		logc(scratch);
		address++;
	}
}

void log(string str)
{
	str = " " + str + "\n";
	DWORD scratch;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), str.c_str(), str.length(), &scratch, 0);
}

void logc(char* str)
{
	log(string(str));
}

void logline()
{
	log("------------------------------------------------------------------------------");
}

string getInput()
{
	char* buffer = new char[1024];
	DWORD written;
	ReadConsoleA(GetStdHandle(STD_INPUT_HANDLE), buffer, 1023, &written, 0);
	buffer[written - 2] = 0;
	string result = string(buffer);
	delete[] buffer;
	return result;
}

void title()
{
	log("");
	log("888 88e   ,e,          888                  ,e,   d8   888");
	log("888 888b   \"   e88'888 888 ee Y8b Y8b Y888P  \"   d88   888 ee");
	log("888 8888D 888 d888  '8 888 P   Y8b Y8b Y8P  888 d88888 888 88b");
	log("888 888P  888 Y888   , 888 b    Y8b Y8b \"   888  888   888 888");
	log("888 88\"   888  \"88,e8' 888 8b    YP  Y8P    888  888   888 888");
	log("");
}

void help()
{
	logline();
	log("Commands: hook / log <address>,  hook / log api <module> <function>");
	log("          unhook <address>");
	log("          call <address> <args>, call api <module> <function> <args>");
	log("          hooks,                 quit / q / exit");
	logline();
}
