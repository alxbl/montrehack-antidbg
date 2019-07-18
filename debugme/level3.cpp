// Level 3: Create a thread that keeps throwing breakpoints at the debuger.
// Solution: Kill the thread and manually set the value of the decryption key when the breakpoint is thrown, then continue.
#include "common.h"

DWORD WINAPI annoyance(LPVOID param) {
    while (1) {
        __try {
            __debugbreak();
        }
        __except (GetExceptionCode() == EXCEPTION_BREAKPOINT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {}
    }
    return 0;
}

void level3()
{
    #ifndef _DEBUG
    __try {
        CreateThread(NULL, NULL, annoyance, NULL, NULL, NULL);
        __debugbreak();
    }
    __except (GetExceptionCode() == EXCEPTION_BREAKPOINT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        keys[2] = KEY3;
    }
    #else
    keys[2] = KEY3;
    #endif  
    cout << "[*] Level 3... ";
    flag(3);
}