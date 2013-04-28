//#ifndef CONSOLE_H
//#define CONSOLE_H

#include <string>
using namespace std;
#include "windows.h"

void logvoid(char* name, DWORD val);
void dump0(DWORD* address, int dwords);
void log(string str);
void logc(char* str);
void logline();
string getInput();
void title();
void help();

//#endif
