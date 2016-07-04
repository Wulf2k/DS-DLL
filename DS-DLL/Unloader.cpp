#include "Unloader.h"

void Unloader::Initialize(HMODULE _hModule)
{
	hModule(_hModule);
}

void Unloader::UnloadSelf(bool bUseThread)
{
	// Useful if we're calling unload from the main game thread.
	if (bUseThread)
	{
		if (!CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Unloader::Unload, Unloader::hModule(NULL), NULL, NULL))
		{
			MessageBox(0, L"Failed to create unload thread.", L"Error", MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		Unload();
	}
}

void __declspec(noreturn) Unloader::Unload()
{
	FreeLibraryAndExitThread(Unloader::hModule(NULL), 0);
}

HMODULE Unloader::hModule(HMODULE _hModule)
{
	static HMODULE hModule;

	if (_hModule)
		hModule = _hModule;

	return hModule;
}