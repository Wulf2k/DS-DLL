#include "DllMain.h"
#include "Project.h"

#include <process.h>
#include <filesystem>

HMODULE hDll = 0;

volatile bool bInitializeCalled = false;

using namespace std;
using namespace tr2::sys;

// Pre-declarations

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID /*lpReserved*/
					 )
{

	if (DLL_PROCESS_ATTACH == ul_reason_for_call)
	{
		//DisableThreadLibraryCalls(hModule);
		hDll = hModule;

		// Guard against multiple calls.
		if (bInitializeCalled) 
		{
			// Decremented reference count.
			FreeLibrary(hModule);
			return FALSE;
		}

		bInitializeCalled = true;

		// Don't do anything important in dll main, just start a new thread, and do your stuff there
		_beginthread(&Start, 0, 0);
	}

	if (DLL_PROCESS_DETACH == ul_reason_for_call)
	{
	}

	return TRUE;
}