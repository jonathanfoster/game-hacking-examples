#include <iostream>
#include <windows.h>

DWORD PrintString(int times, const char* string)
{
    for (int i = 0; i < times; i++)
    {
        printf(string);
    }

    return 0;
}

void InjectUsingThreadInjection(HANDLE process, LPVOID func, int times, const char* string)
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

    int stringLength = strlen(string) + 1;
    int fullLength = stringLength + sizeof(codeCave);
    
    // Allocate memory for code cave
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

int main()
{
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    InjectUsingThreadInjection(process, &PrintString, 1, "Injected!\n");

    return 0;
}
