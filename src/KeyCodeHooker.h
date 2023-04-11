#pragma once

#include <Windows.h>

#if defined(KeyCodeHooker_EXPORTS)
#define KEYCODEHOOKER_EXPORT __declspec(dllexport)
#else
#define KEYCODEHOOKER_EXPORT __declspec(dllimport)
#endif

class KEYCODEHOOKER_EXPORT KeyCodeHooker
{
public:
    virtual LRESULT OnHookProc(int nCode, WPARAM wParam, LPARAM lParam) = 0;
};

KEYCODEHOOKER_EXPORT void HookStart(KeyCodeHooker* hooker);
KEYCODEHOOKER_EXPORT void HookStop();
