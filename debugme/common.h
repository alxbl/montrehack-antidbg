#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include "crypto.h"

using namespace std; // Lol.

#define KEY1 0xabcd1234DCBA4321
#define KEY2 0x1122334455667788
#define KEY3 0xbaba848401020300
#define KEY4 0x34674811a
#define KEY5 0x7001337

#define KEY 0xb4adbeeff331b44d
#define LEN 40 // 32 + 8 for DES padding block.
typedef void(*LevelFn)(void);

// Forward declarations
extern HWND hWnd;
extern HANDLE evtOnBreakPoint;
extern HANDLE evtOnSegfault;
extern const char* flags[];

extern uint64_t keys[5];
void level1();
void level2();
void level3();
void level4();
void level5();
DWORD level5_prep();

__inline void flag(int n) {
    auto key = md5((byte_t*)& keys[n-1], 8);
    auto c = decrypt(key, (byte_t*)flags[n-1], LEN);
    printf("Your flag is at 0x%016IX\n", (uint64_t)c->data);
    bfree(c);
    bfree(key);
}

// Disharm anti-debugging for debug builds :)
#ifdef _DEBUG
#define exit(n) cout << "EXIT: " << n << endl
#define IsDebuggerPresent() false
void encrypt_flags();
#endif
