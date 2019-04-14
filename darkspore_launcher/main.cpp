// Reference:
// https://www.unknowncheats.me/forum/c-and-c-/72416-simplest-function-hooking-ida-detours.html
// https://github.com/saeedirha/DLL-Injector

#include <windows.h>
#include <iostream>
#include <string>
#include <ctype.h>
#include <tlhelp32.h>

#include "detours.h"

using namespace std;

int (__thiscall* copyStringFunction)(int,void*,int) = (int(__thiscall*)(int,void*,int))(0x402270);
BYTE* (__thiscall* getConfigsFunction)(BYTE*) = (BYTE*(__thiscall*)(BYTE*))(0x4642E0);

int copyStringHookedFunction(int v1, void *Src, int a3)
{
  return copyStringFunction(v1, Src, a3);
}

BYTE* getConfigsHookedFunction(BYTE* v1)
{
  BYTE* b = getConfigsFunction(v1);
  void* str = const_cast< char* >("http://127.0.0.1/bootstrap/api?version=1");
  copyStringHookedFunction((int)(v1 + 56), str, (int)"");
  return b;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      DetourTransactionBegin();
      DetourUpdateThread(GetCurrentThread());
      copyStringFunction = (int(__thiscall*)(int,void*,int))DetourAttach((PVOID*)(&copyStringFunction), (PBYTE)copyStringHookedFunction);
      getConfigsFunction = (BYTE*(__thiscall*)(BYTE*))DetourAttach((PVOID*)(&getConfigsFunction), (PBYTE)getConfigsHookedFunction);
      DetourTransactionCommit();
      break;
  }
  return TRUE;
}



int getProcID()
{
  std::string p_name ("Darkspore.exe");
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 structprocsnapshot = { 0 };

	structprocsnapshot.dwSize = sizeof(PROCESSENTRY32);

	if (snapshot == INVALID_HANDLE_VALUE)return 0;
	if (Process32First(snapshot, &structprocsnapshot) == FALSE)return 0;

	while (Process32Next(snapshot, &structprocsnapshot))
	{
		if (!strcmp(structprocsnapshot.szExeFile, p_name.c_str()))
		{
			CloseHandle(snapshot);
			cout << "[+]Process name is: " << p_name << "\n[+]Process ID: " << structprocsnapshot.th32ProcessID << endl;
			return structprocsnapshot.th32ProcessID;
		}
	}
	CloseHandle(snapshot);
	cerr << "[!]Unable to find Process ID" << endl;
	return 0;

}

bool InjectDLL(const int &pid, const string &DLL_Path)
{
	long dll_size = DLL_Path.length() + 1;
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	if (hProc == NULL)
	{
		cerr << "[!]Fail to open target process!" << endl;
		return false;
	}
	cout << "[+]Opening Target Process..." << endl;

	LPVOID MyAlloc = VirtualAllocEx(hProc, NULL, dll_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (MyAlloc == NULL)
	{
		cerr << "[!]Fail to allocate memory in Target Process." << endl;
		return false;
	}

	cout << "[+]Allocating memory in Targer Process." << endl;
	int IsWriteOK = WriteProcessMemory(hProc , MyAlloc, DLL_Path.c_str() , dll_size, 0);
	if (IsWriteOK == 0)
	{
		cerr << "[!]Fail to write in Target Process memory." << endl;
		return false;
	}
	cout << "[+]Creating Remote Thread in Target Process" << endl;

	DWORD dWord;
	LPTHREAD_START_ROUTINE addrLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(LoadLibrary("kernel32"), "LoadLibraryA");
	HANDLE ThreadReturn = CreateRemoteThread(hProc, NULL, 0, addrLoadLibrary, MyAlloc, 0, &dWord);
	if (ThreadReturn == NULL)
	{
		cerr << "[!]Fail to create Remote Thread" << endl;
		return false;
	}

	if ((hProc != NULL) && (MyAlloc != NULL) && (IsWriteOK != ERROR_INVALID_HANDLE) && (ThreadReturn != NULL))
	{
		cout << "[+]DLL Successfully Injected :)" << endl;
		return true;
	}

	return false;
}

int main(int argc, char ** argv)
{
  system("Darkspore.exe");
  InjectDLL(getProcID(), argv[0]);
}
