#pragma once

#include <Windows.h>

class Unloader
{
public:
	static void Initialize(HMODULE _hModule);
	static void UnloadSelf(bool bUseThread);

private:

	static void __declspec(noreturn) Unload();

	// This avoids around having to define hModule outside of the header.
	static HMODULE hModule(HMODULE _hModule);
};