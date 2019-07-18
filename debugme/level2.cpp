#include "common.h"

DWORD WINAPI level2_watchdog(LPVOID param) {
    while (1) {
        #ifndef _DEBUG
        HANDLE h = GetProcessHeap();
        DWORD_PTR* f = (DWORD_PTR*)((char*)h + 0x74); // _HEAP->ForceFlags should be 0 unless a debugger spawned the process.
        
        if (*f != 0) {
            OutputDebugStringA("Heap Corruption Detected.");
            auto fn = (LevelFn)nullptr; fn(); // Misdirection.
        }
        #endif  
        keys[1] = KEY2;
        SetEvent(evtOnBreakPoint);
    }
    return 0;
}

// Level 2: Spawns a thread that keeps probing the heap flags and exits if they have been modified.
// Solution: Kill/Patch out the thread and WaitForSingleObject.
void level2() {
    cout << "[*] Level 2... ";
    CreateThread(NULL, NULL, level2_watchdog, NULL, NULL, NULL);
    WaitForSingleObject(evtOnBreakPoint, -1);
    flag(2);
}
