#include <iostream>
#include <windows.h>
#include <TlHelp32.h>

DWORD PrintString(int times, const char* string)
{
    for (int i = 0; i < times; i++)
    {
        printf(string);
    }

    return 0;
}

void InjectCodeUsingThreadInjection(HANDLE process, LPVOID func, int times, const char* string)
{
    BYTE codeCave[20] = {
        0xFF, 0x74, 0x24, 0x04,         // PUSH DWORD PTR[ESP+0x4]
        0x68, 0x00, 0x00, 0x00, 0x00,   // PUSH 0x0
        0xB8, 0x00, 0x00, 0x00, 0x00,   // MOV EAX, 0x0
        0xFF, 0xD0,                     // CALL EAX
        0x83, 0xC4, 0x08,               // ADD ESP, 0x08
        0xC3                            // RETN
    };

    // Copy values to code cave
    memcpy(&codeCave[5], &times, 4);    // PUSH &times
    memcpy(&codeCave[10], &func, 4);    // MOV EAX, &func

    // Allocate memory for code cave
    int stringLength = strlen(string) + 1;
    int fullLength = stringLength + sizeof(codeCave);
    LPVOID remoteString = VirtualAllocEx(process, NULL, fullLength, MEM_COMMIT, PAGE_EXECUTE);
    LPVOID remoteCave = (LPVOID)((DWORD)remoteString + stringLength);

    // Write code cave
    WriteProcessMemory(process, remoteString, string, stringLength, NULL);
    WriteProcessMemory(process, remoteCave, codeCave, sizeof(codeCave), NULL);

    // Run thread
    HANDLE thread = CreateRemoteThread(process, NULL, NULL, (LPTHREAD_START_ROUTINE)remoteCave, remoteString, NULL, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

DWORD GetProcessThreadId(HANDLE process)
{
    THREADENTRY32 entry;
    entry.dwSize = sizeof(THREADENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    if (Thread32First(snapshot, &entry) == TRUE) 
    {
        DWORD pid = GetProcessId(process);

        while (Thread32Next(snapshot, &entry) == TRUE)
        {
            if (entry.th32OwnerProcessID == pid)
            {
                CloseHandle(snapshot);
                return entry.th32ThreadID;
            }
        }
    }

    CloseHandle(snapshot);
    return NULL;
}

void InjectCodeUsingThreadHijacking(HANDLE process, LPVOID func, int times, const char* string)
{
    BYTE codeCave[31] = {
        0x60,                           // PUSHAD
        0x9C,                           // PUSHFD
        0x68, 0x00, 0x00, 0x00, 0x00,   // PUSH 0x0
        0x68, 0x00, 0x00, 0x00, 0x00,   // PUSH 0x0
        0xB8, 0x00, 0x00, 0x00, 0x00,   // MOV EAX, 0x0
        0xFF, 0xD0,                     // CALL EAX
        0x83, 0xC4, 0x08,               // ADD ESP, 0x08
        0x9D,                           // POPFD
        0x61,                           // POPAD
        0x68, 0x00, 0x00, 0x00, 0x00,   // PUSH 0x0
        0xC3                            // RETN
    };
    
    // Allocate memory for code cave
    int stringLength = strlen(string) + 1;
    int fullLength = stringLength + sizeof(codeCave);
    LPVOID remoteString = VirtualAllocEx(process, NULL, fullLength, MEM_COMMIT, PAGE_EXECUTE);
    LPVOID remoteCave = (LPVOID)((DWORD)remoteString + stringLength);

    // Suspend thread and query control context
    DWORD threadId = GetProcessThreadId(process);
    HANDLE thread = OpenThread((THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_SET_CONTEXT), false, threadId);
    SuspendThread(thread);

    CONTEXT threadContext;
    threadContext.ContextFlags = CONTEXT_CONTROL;
    GetThreadContext(thread, &threadContext);

    // Copy values to code cave
    memcpy(&codeCave[3], &remoteString, 4);         // PUSH 0x0
    memcpy(&codeCave[8], &times, 4);                // PUSH 0x0
    memcpy(&codeCave[13], &func, 4);                // MOV EAX, 0x0
    memcpy(&codeCave[25], &threadContext.Eip, 4);   // MOV EAX, 0x0

    // Write code cave
    WriteProcessMemory(process, remoteString, string, stringLength, NULL);
    WriteProcessMemory(process, remoteCave, codeCave, sizeof(codeCave), NULL);

    // Hijack thread
    threadContext.Eip = (DWORD)remoteCave;
    threadContext.ContextFlags = CONTEXT_CONTROL;
    SetThreadContext(thread, &threadContext);
    ResumeThread(thread);
    CloseHandle(thread);
}

DWORD WINAPI HijackThread(LPVOID lpParameter)
{
    InjectCodeUsingThreadHijacking((HANDLE)lpParameter, &PrintString, 1, "Hijacked!\n");

    return 0;
}

int main()
{
    std::cout << "Injecting code using thread injection\n";
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    InjectCodeUsingThreadInjection(process, &PrintString, 1, "Injected!\n");

    // Hijack a secondary thread because hijacking current thread won't work
    std::cout << "Injecting code using thread hijacking\n";
    HANDLE thread = CreateThread(NULL, 0, HijackThread, process, 0, NULL);
        
    // Wait for thread to complete
    WaitForSingleObject(thread, INFINITE);

    return 0;
}
