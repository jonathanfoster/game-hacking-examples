#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>

DWORD GetProcessIdFromFileName(std::wstring fileName)
{
    DWORD pid;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            std::wstring binPath = entry.szExeFile;

            if (binPath.find(fileName) != std::wstring::npos) 
            {
                pid = entry.th32ProcessID;
                break;
            }
        }
    }

    CloseHandle(snapshot);

    return pid;
}

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

    // Get window name
    wchar_t windowName[1024];
    GetConsoleTitle(&windowName[0], 1024);

    DWORD pid = GetProcessIdFromWindowName(windowName);
    printf("PID from window name is %d\n", pid);

    pid = GetProcessIdFromFileName(L"ReadWriteMemory.exe");
    printf("PID from file name is %d\n", pid);

    return 0;
}
