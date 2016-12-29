#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>

DWORD GetProcessIdFromWindowName(LPCWSTR windowName)
{
    DWORD pid;
    HWND window = FindWindow(NULL, windowName);
    GetWindowThreadProcessId(window, &pid);
    return pid;
}

int main()
{
    HANDLE process = OpenProcess(
        PROCESS_VM_OPERATION | 
        PROCESS_VM_READ | 
        PROCESS_VM_WRITE | 
        PROCESS_CREATE_THREAD, 
        FALSE, 
        GetCurrentProcessId());

    // Get current window name
    wchar_t windowName[1024];
    GetConsoleTitle(&windowName[0], 1024);

    DWORD pid = GetProcessIdFromWindowName(windowName);
    printf("Current PID is %d\n", pid);

    return 0;
}
