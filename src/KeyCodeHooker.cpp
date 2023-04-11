#include "KeyCodeHooker.h"

HHOOK hHook = NULL;
KeyCodeHooker* pHooker = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
    {
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    }
    if (pHooker->OnHookProc(nCode, wParam, lParam))
    {
        return 1;
    }
    else
    {
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    }
}

void HookStart(KeyCodeHooker* hooker)
{
    HookStop();

    if (hooker)
    {
        pHooker = hooker;
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    }

    if (hHook)
    {
        OutputDebugString(TEXT("Hook Installed\n"));
    }
}

void HookStop()
{
    if (hHook)
    {
        UnhookWindowsHookEx(hHook);

        hHook = NULL;
        pHooker = NULL;

        OutputDebugString(TEXT("Hook Uninstalled\n"));
    }
}
