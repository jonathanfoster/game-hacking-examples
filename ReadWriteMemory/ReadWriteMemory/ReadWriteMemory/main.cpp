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

DWORD GetBaseAddress(HANDLE process)
{
    DWORD baseAddress;

    HMODULE module = GetModuleHandle(L"kernel32.dll");
    LPVOID func=  GetProcAddress(module, "GetModuleHandleA");
    if (!func)
    {
        func = GetProcAddress(module, "GetModuleHandleW");
    }

    HANDLE thread = CreateRemoteThread(process, NULL, NULL, (LPTHREAD_START_ROUTINE)func, NULL, NULL, NULL);
    WaitForSingleObject(thread, INFINITE);
    GetExitCodeThread(thread, &baseAddress);
    CloseHandle(thread);

    return baseAddress;
}

template<typename T>
T ReadMemory(HANDLE process, LPVOID address)
{
    T value;
    ReadProcessMemory(process, address, sizeof(T), NULL);
    return value;
}

template<typename T>
void WriteMemory(HANDLE process, LPVOID address, T value)
{
    WriteProcessMemory(process, address, &value, sizeof(T), NULL);
}

template<typename T>
DWORD ProtectMemory(HANDLE process, LPVOID address, DWORD newProtection)
{
    DWORD oldProtection;
    VirtualProtectEx(process, address, sizeof(T), newProtection, &oldProtection);
    return oldProtection;
}

int main(int argc, char** argv)
{
    // Get current window name
    wchar_t windowName[1024];
    GetConsoleTitle(&windowName[0], 1024);

    DWORD pid = GetProcessIdFromWindowName(windowName);
    printf("PID from window name is %d\n", pid);

    pid = GetProcessIdFromFileName(L"ReadWriteMemory.exe");
    printf("PID from file name is %d\n", pid);

    HANDLE process = OpenProcess(
        PROCESS_VM_OPERATION |
        PROCESS_VM_READ |
        PROCESS_VM_WRITE |
        PROCESS_CREATE_THREAD,
        FALSE,
        pid);

    DWORD baseAddress = GetBaseAddress(process);
    printf("PID %d base address is 0x%08X\n", pid, baseAddress);

    return 0;
}
