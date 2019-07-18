// The trickiest level:
//
#include "common.h"
#include "crypto.h"

#define SIZE 0x100
#define PIPE TEXT("\\\\.\\pipe\\wawawa")
HANDLE hPipe = nullptr;


DWORD level5_prep() {
    #ifdef _DEBUG
    keys[4] = KEY5;
    return 0;
    #endif
    hPipe = CreateNamedPipe(
        PIPE,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        SIZE,
        SIZE,
        0,
        NULL
    );

    auto gle = GetLastError();

    // DEBUGGER PROCESS
    if (hPipe == INVALID_HANDLE_VALUE || gle == ERROR_ACCESS_DENIED) {
        hPipe = CreateFile(
            PIPE,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
        );

        // Read the PID from the pipe.
        DWORD pid, num;
        uint64_t key = 0xf0faf0f0fafafeee;
        ReadFile(hPipe, (void*)& pid, sizeof(DWORD), &num, NULL);

        // Attach the debugger. If it fails, send a bogus key.
        // TODO: On a separate thread?
        if (DebugActiveProcess(pid)) {
            DebugSetProcessKillOnExit(false);
            DebugActiveProcessStop(pid);
            key = KEY5;
        }
        WriteFile(hPipe, (void*)& key, sizeof(key), &num, NULL);
        CloseHandle(hPipe);
        return pid;
    }

    // HOST PROCESS
    if (hPipe == INVALID_HANDLE_VALUE) return 10;
    // Spawn the debugger process.
    TCHAR szFileName[MAX_PATH + 1];
    GetModuleFileName(NULL, szFileName, MAX_PATH + 1);
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    GetStartupInfo(&si);
    CreateProcess(szFileName, NULL, NULL, NULL, NULL, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);

    // Receive debugger connection.
    ConnectNamedPipe(hPipe, NULL);
    auto pid = GetCurrentProcessId();
    DWORD num;
    WriteFile(hPipe, (void*)& pid, sizeof(pid), &num, NULL);

    // Receive the Key.
    uint64_t key;
    ReadFile(hPipe, (void*)& key, sizeof(key), &num, NULL);
    CloseHandle(hPipe);
    keys[4] = key;
    return 0;
}


void level5()
{
    cout << "[*] Level 5... ";
    flag(5);
}
