#include "crypto.h"
#include "common.h"

HWND   hWnd            = nullptr;
HANDLE evtOnBreakPoint = nullptr;
HANDLE evtOnSegfault    = nullptr;

LevelFn levels[] = { level1, level2, level3, level4, level5, };
uint64_t keys[5] = { 0 };

const char* flags[] = { // Pad raw flags to LEN 32 for simplicity, use LEN 40 for encrypted flags.
    "\xb9\x47\x5c\x2a\x1f\xb0\xb3\x96\xc8\xbc\xf3\x1c\x0d\x8f\xc6\xa5\x53\xbf\x14\x9a\x84\x33\x5f\xa0\xb4\xd8\xab\xbc\x3b\xe3\x66\xf3\x27\x64\x88\xa1\x94\x2c\x13\xd8\x00\xb0\x0e\x00\x00\x00\x00\x00", // 0: FLAG-{n0pping_1n_th3_90s}        | KEY=abcd1234dcba4321
    "\x98\x77\xae\xbe\x76\x3e\x3b\xb3\x8a\xb7\x8e\xc6\xe7\xca\x97\x31\x6c\x03\x82\x90\xeb\x87\xba\x55\x08\x8c\xaa\x37\x30\x37\x9d\xaf\x4a\xf5\xb1\xbd\xde\x5c\x1e\xb7\x00\x00\x00\x00\x00\x00\x00\x00", // 1: FLAG-{h3aps_of_l1ez_4wa1t}       | KEY=1122334455667788
    "\xf9\xa1\x96\xed\x7e\xdb\x36\xdb\x5d\xaa\x01\x95\xd1\x8a\x6e\xca\x27\xdc\x1f\x45\x99\x8f\xf4\xf1\xa5\x5a\xb7\x53\x3b\xb9\xb3\xce\x50\x76\x6f\xdb\x04\x43\xc8\xfd\x00\x00\x00\x00\x00\x00\x00\x00", // 2: FLAG-{U_k4nT_brk_m3_50_3aZy}     | KEY=baba848401020300
    "\x75\x9d\x9d\x3c\xc5\x83\xad\x8b\xbf\x47\x99\x7c\x10\xbb\xc0\x7c\x87\xe9\x13\x3e\x5f\x9b\x4e\x23\x12\x6c\x06\x75\x0a\x67\xd0\x72\x9c\x56\x28\xda\xd8\xe0\x4e\x8f\x00\x00\x00\x00\x00\x00\x00\x00", // 3: FLAG-{G3tT1nG_5eri0us_n0w}       | KEY=000000034674811a
    "\x3a\x44\x17\x23\xa7\x95\x63\xf0\x1a\x02\xa0\x2e\xe8\x5e\xca\xa3\xa0\x21\x4a\x91\x0f\x92\x9c\x20\xb6\xed\xb8\x6c\xe2\x87\x00\xa5\xe1\x17\xe6\xc8\x0c\x8b\xef\x20\x00\x00\x00\x00\x00\x00\x00\x00", // 4: FLAG-{sUbv3rt_th3_KrNlz}         | KEY=0000000007001337
};


int main() {
    #ifdef _DEBUG
    // In Debug Mode, encrypt and print the resulting flags array.
    encrypt_flags();
    #endif

    cout << "                 _   _     _ _           " << endl
        << "     /\\         | | (_)   | | |          " << endl
        << "    /  \\   _ __ | |_ _  __| | |__   __ _ " << endl
        << "   / /\\ \\ | '_ \\| __| |/ _` | '_ \\ / _` |" << endl
        << "  / ____ \\| | | | |_| | (_| | |_) | (_| |" << endl
        << " /_/    \\_\\_| |_|\\__|_|\\__,_|_.__/ \\__, |" << endl
        << "                                    __/ |" << endl
        << "                                   |___/ " << endl
        << "              SEGV@Montrehack 2019-07-17 " << endl
        << "       ... Debug me if you can ...       " << endl;

    evtOnBreakPoint = CreateEvent(NULL, FALSE, FALSE, NULL);
    int n = sizeof(levels) / sizeof(LevelFn);
    for (auto i = 0; i < n; ++i) {
        levels[i]();
    }
    return 0;
}

LRESULT CALLBACK wndProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
    if (uMsg == WM_USER) {
        LevelFn f = (LevelFn)((uint64_t)lParam ^ KEY);
        f();
        return 1;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

VOID WINAPI tls_callback(PVOID DllHandle, DWORD Reason, PVOID Reserved) {
    if (Reason != 1) return;
    evtOnSegfault = CreateEvent(NULL, FALSE, TRUE, NULL);
    auto h = GetModuleHandle(nullptr);
    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = wndProc;
    wx.hInstance = h;
    wx.lpszClassName = "WinPwn";

    if (!hWnd && RegisterClassEx(&wx) && !IsDebuggerPresent()) hWnd = CreateWindowEx(0, "WinPwn", "win_pwn", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (level5_prep()) ExitProcess(3);
    SetEvent(evtOnSegfault);
}

// Register the TLS callback.
#ifdef _M_AMD64
#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:p_tls_callback1")
#pragma const_seg(push)
#pragma const_seg(".CRT$XLAAA")
EXTERN_C const PIMAGE_TLS_CALLBACK p_tls_callback1 = tls_callback;
#pragma const_seg(pop)
#endif
#ifdef _M_IX86
#error "Can only build on x64!!"
#endif
