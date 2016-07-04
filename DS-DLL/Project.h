#pragma once

#include <Windows.h>

#ifdef PUBLIC_RELEASE
	#pragma message("Public Release")
	#define DLLEXPORT 
#endif

#ifndef PUBLIC_RELEASE
	#pragma message("Private Release")
	#define DLLEXPORT __declspec(dllexport)
#endif

// Useful enabling exports on development builds. Disabled for release.

DLLEXPORT void __cdecl Initialize(void*);
DLLEXPORT void __cdecl Start(void*);