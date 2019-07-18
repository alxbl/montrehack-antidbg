// This level is trickier. It fakes a large number of access violations which are handled by sending messages to a window.
// The window keeps an internal counter and after a threshold is met, sets pfSetKey to a value that will result in it being called.
// The problem: The Window is only created if IsDebuggerPresent returns false. So it will never process messages if Level 1 is solved by
// simply patching out IsDebuggerPresent or skipping over it.
// Solution:
//  - Patch IsDebuggerPresent to always return 0
//  - Patch pfSetKey so that it gets called on the first try by reversing the window message handler.
#include "common.h"
#define LEVEL4_LEN 103

void level4_key();
void level4_count();

int c5count = 0; // Number of access violations seen.
#ifndef _DEBUG
uint64_t pfSetKey = (uint64_t)level4_key ^ KEY;
uint64_t pfCount = (uint64_t)level4_count ^ KEY;
#else
uint64_t pfSetKey = (uint64_t)level4_key;
uint64_t pfCount = (uint64_t)level4_count;
#endif

void level4_key() {
    keys[3] = KEY4;
}

void level4_count() {
    if (++c5count == LEVEL4_LEN) pfSetKey = (uint64_t)pfSetKey ^ KEY;
}

void level4()
{
    cout << "[*] Level 4... ";
    bool x = false;
    uint64_t orig = pfSetKey;
    WaitForSingleObject(evtOnSegfault, -1);
    while (!x) {
        __try {
            ((LevelFn)pfSetKey)();
            x = true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            if (!hWnd || !SendMessage(hWnd, WM_USER, NULL, pfCount)) x = true;
            if (pfSetKey != orig) {
                x = true;
                ((LevelFn)pfSetKey)();
            }
        }
    }
    flag(4);
}