#include "Console.h"
#include <Windows.h>
#include <cstdio>

void Console::Create(const char* name)
{
	AllocConsole();
	SetConsoleTitleA(name);
	RedirectOutput();
	RedirectInput();
}

void Console::Free()
{
	FreeConsole();
	RestoreOutput();
	RestoreInput();
}

void Console::RedirectOutput()
{
	// Redirect stdout stream.
	FILE* stream;
	errno_t err = freopen_s(&stream, "CONOUT$", "w+", stdout);

	DBG_UNREFERENCED_LOCAL_VARIABLE(err);
}

void Console::RedirectInput()
{
	// Redirect stdin stream.
	FILE* stream;
	errno_t err = freopen_s(&stream, "CONIN$", "r", stdin);

	DBG_UNREFERENCED_LOCAL_VARIABLE(err);
}

void Console::RestoreOutput()
{
	// Restore stdout to default.
	FILE* stream;
	freopen_s(&stream, "OUT", "w", stdout);
}

void Console::RestoreInput()
{
	// Restore stdout to default.
	FILE* stream;
	freopen_s(&stream, "IN", "r", stdin);
}