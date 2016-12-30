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
    ReadProcessMemory(process, address, &value, sizeof(T), NULL);
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

int main()
{
    // Get current window name
    wchar_t windowName[1024];
    GetConsoleTitle(&windowName[0], 1024);

    DWORD pid = GetProcessIdFromWindowName(windowName);
    printf("PID from current window name is %d\n", pid);

    pid = GetProcessIdFromFileName(L"ReadWriteMemory.exe");
    printf("PID from current file name is %d\n", pid);

    HANDLE process = OpenProcess(
        PROCESS_VM_OPERATION |
        PROCESS_VM_READ |
        PROCESS_VM_WRITE |
        PROCESS_CREATE_THREAD,
        FALSE,
        pid);
    
    DWORD baseAddress = GetBaseAddress(process);
    printf("PID %d base address is 0x%016X\n", pid, baseAddress);

    // Don't forget to start the remote process
    // TODO: Automatically start remote process
    pid = GetProcessIdFromFileName(L"ReadWriteMemoryExe.exe");
    printf("PID from remote file name is %d\n", pid);

    process = OpenProcess(
        PROCESS_VM_OPERATION |
        PROCESS_VM_READ |
        PROCESS_VM_WRITE |
        PROCESS_CREATE_THREAD,
        FALSE,
        pid);

    baseAddress = GetBaseAddress(process);
    printf("PID %d base address is 0x%016X\n", pid, baseAddress);

    // You'll need to rediscover this addresses if you recompile the remote process
    LPVOID secretAddress = (LPVOID)0x199004;

    int secret = ReadMemory<int>(process, secretAddress);

    printf("Secret is %d\n", secret);

    WriteMemory<int>(process, secretAddress, 1234567890);
    int newSecret = ReadMemory<int>(process, secretAddress);

    printf("New secret is %d\n", newSecret);

    // Don't forget to kill the remote process
    // TODO: Automatically kill remote process
    return 0;
}
