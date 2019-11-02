// Hook.cpp : Defines the entry point for the console application.
// this will serve as our target application
#include <Windows.h>
#include <iostream>

// this is the function that will be hooked by our dll:
int sum(int x, int y)
{
	return x + y;
}

int main()
{
	while (true)
	{
		// this will print out the result of 5 + 5 once a second
		std::cout << "testprogram.exe: 5 + 5 = " << sum(5, 5) << std::endl;
		Sleep(1000);
	}
}
