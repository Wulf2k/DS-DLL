#pragma once

class Console
{
public:
	static void Create(const char* name);
	static void Free();

private:

	static void RedirectOutput();
	static void RedirectInput();

	static void RestoreOutput();
	static void RestoreInput();
};