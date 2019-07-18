#include "common.h"

// Level 1:  Checks is debugger present and if so, exits. 
// Solution: Patch that call out or make it always return false.
void level1() {
    keys[0] = KEY1;
    cout << "[*] Level 1... ";
    if (IsDebuggerPresent()) {
        OutputDebugString("I see you, hacker!\n");
        exit(0x7001a7e);
    }
    flag(1);
}
