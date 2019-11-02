#include <Windows.h>
#include <iostream>

#include "detours.h"
#include "sigscan.h"

DWORD AddressOfGetConfigsFunction = 0; typedef BYTE* (*getConfigsFunction)(BYTE* v1);
DWORD AddressOfCopyStringFunction = 0; typedef int(__thiscall* copyStringFunction)(int v1, void* v2, int v3);

BYTE* getConfigsHookedFunction(BYTE* v1)
{
	getConfigsFunction getConfigs = (getConfigsFunction)AddressOfGetConfigsFunction;
	copyStringFunction copyString = (copyStringFunction)AddressOfCopyStringFunction;
	
	BYTE* b = getConfigs(v1);
	void* str = const_cast<char*>("http://127.0.0.1/bootstrap/api?version=1");
	copyString((int)(v1 + 56), str, (int)"");

	return b;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		SigScan Scanner;

		AddressOfGetConfigsFunction = Scanner.FindPattern("Darkspore.exe", "\xCB\xFF\xD0\x8B\x4E\x28\x8B\x11\x8B");
		AddressOfCopyStringFunction = Scanner.FindPattern("Darkspore.exe", "\x00\x00\x00\xEB\x16\xF3\x0F\x10\x41");

		//AddressOfGetConfigsFunction = Scanner.GetAddress("Darkspore.exe", 0x4642E0);
		//AddressOfCopyStringFunction = Scanner.GetAddress("Darkspore.exe", 0x402270);

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetourAttach(&(LPVOID&)AddressOfGetConfigsFunction, &getConfigsHookedFunction);

		DetourTransactionCommit();
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetourDetach(&(LPVOID&)AddressOfGetConfigsFunction, &getConfigsHookedFunction);

		DetourTransactionCommit();
	}
	return TRUE;
}