#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>

void LogMessage3(const char *message)
{
	// return;

#ifdef _WIN32
	const char *log_file = "c:\\temp\\esp_adapter_dll_main.log";
#else
	const char *log_file = "/home/serega/esp_adapter_dll_main.log";
#endif

	FILE *f = fopen(log_file, "at");
	fputs(message, f); 
	fputs("\n", f);
	fclose(f);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		LogMessage3("DLL_PROCESS_ATTACH");
		break;
	case DLL_THREAD_ATTACH:
		LogMessage3("DLL_THREAD_ATTACH");
		break;
	case DLL_THREAD_DETACH:
		LogMessage3("DLL_THREAD_DETACH");
		break;
	case DLL_PROCESS_DETACH:
		LogMessage3("DLL_PROCESS_DETACH");
		break;
	}

	return TRUE;
}
