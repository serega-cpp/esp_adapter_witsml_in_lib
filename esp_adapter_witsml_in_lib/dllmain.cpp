#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include "log_message.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		log_message("DllMain", "DLL_PROCESS_ATTACH");
		break;
	case DLL_THREAD_ATTACH:
		log_message("DllMain", "DLL_THREAD_ATTACH");
		break;
	case DLL_THREAD_DETACH:
		log_message("DllMain", "DLL_THREAD_DETACH");
		break;
	case DLL_PROCESS_DETACH:
		log_message("DllMain", "DLL_PROCESS_DETACH");
		break;
	}

	return TRUE;
}
