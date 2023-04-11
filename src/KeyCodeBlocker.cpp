#include "KeyCodeBlocker.h"
#include "KeyCodeHooker.h"

#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <vector>

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

constexpr int clientWidth = 960;
constexpr int clientHeight = 275;

constexpr TCHAR szTitle[] = TEXT("KeyCode Blocker");
constexpr TCHAR szWindowClass[] = TEXT("KeyCodeBlocker");

const TCHAR* KeyCapLabels[256] =
{
    // [0]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT("BACKSPACE"),TEXT("TAB"),
    // [10]
    TEXT(""),TEXT(""),TEXT(""),TEXT("↩"),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("PAUSE"),
    // [20]
    TEXT("CAPS LOCK"),TEXT("한/영"),TEXT(""),TEXT(""),TEXT(""),
    TEXT("한자"),TEXT(""),TEXT("ESC"),TEXT(""),TEXT(""),
    // [30]
    TEXT(""),TEXT(""),TEXT("SPACE"),TEXT("PGUP"),TEXT("PGDN"),
    TEXT("END"),TEXT("HOME"),TEXT("←"),TEXT("↑"),TEXT("→"),
    // [40]
    TEXT("↓"),TEXT(""),TEXT(""),TEXT(""),TEXT("PRINT"),
    TEXT("INS"),TEXT("DEL"),TEXT(""),TEXT("0"),TEXT("1"),
    // [50]
    TEXT("2"),TEXT("3"),TEXT("4"),TEXT("5"),TEXT("6"),
    TEXT("7"),TEXT("8"),TEXT("9"),TEXT(""),TEXT(""),
    // [60]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT("A"),TEXT("B"),TEXT("C"),TEXT("D"),TEXT("E"),
    // [70]
    TEXT("F"),TEXT("G"),TEXT("H"),TEXT("I"),TEXT("J"),
    TEXT("K"),TEXT("L"),TEXT("M"),TEXT("N"),TEXT("O"),
    // [80]
    TEXT("P"),TEXT("Q"),TEXT("R"),TEXT("S"),TEXT("T"),
    TEXT("U"),TEXT("V"),TEXT("W"),TEXT("X"),TEXT("Y"),
    // [90]
    TEXT("Z"),TEXT("WIN"),TEXT("WIN"),TEXT(""),TEXT(""),
    TEXT(""),TEXT("0"),TEXT("1"),TEXT("2"),TEXT("3"),
    // [100]
    TEXT("4"),TEXT("5"),TEXT("6"),TEXT("7"),TEXT("8"),
    TEXT("9"),TEXT("*"),TEXT("+"),TEXT(""),TEXT("-"),
    // [110]
    TEXT("."),TEXT("/"),TEXT("F1"),TEXT("F2"),TEXT("F3"),
    TEXT("F4"),TEXT("F5"),TEXT("F6"),TEXT("F7"),TEXT("F8"),
    // [120]
    TEXT("F9"),TEXT("F10"),TEXT("F11"),TEXT("F12"),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [130]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [140]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("NUM"),
    TEXT("SCRLK"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [150]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [160]
    TEXT("SHIFT"),TEXT("SHIFT"),TEXT("CTRL"),TEXT(""),TEXT("ALT"),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [170]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [180]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(";"),TEXT("="),TEXT(","),TEXT("-"),
    // [190]
    TEXT("."),TEXT("/"),TEXT("`"),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [200]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [210]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("{"),
    // [220]
    TEXT("\\"),TEXT("}"),TEXT("'"),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [230]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [240]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    // [250]
    TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT(""),
    TEXT(""),
};

LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct KeyCap
{
    int keyCode;

    int row;
    int rowSpan;
    int column;
    int group;
    int section;
    float offset;
    float weight;
    float padding; // 단차에 따른 영문자 키 갯수차이에 다른 키의 너비에 일정 padding 값을 더하기 위함 (백분율)
    float paddingSum;

    RECT rect;
};

class KeyCodeBlocker : private KeyCodeHooker
{
public:
    KeyCodeBlocker()
        : width(0)
        , height(0)
        , lock{}
        , press{}
        , counter{}
        , hWnd(NULL)
    {
        lock[VK_LWIN] = true;
        lock[VK_RWIN] = true;

        labelFont = CreateFont(14, 0, 0, 0, FW_BOLD, 0, 0, 0, HANGEUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("맑은 고딕"));
        counterFont = CreateFont(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, HANGEUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, TEXT("맑은 고딕"));

        lockNormalBrush = CreateSolidBrush(RGB(0x66, 0x22, 0x22));
        lockPressedBrush = CreateSolidBrush(RGB(0xF0, 0x80, 0x80));
        unlockNormalBrush = CreateSolidBrush(RGB(0x3D, 0x3D, 0x3D));
        unlockPressedBrush = CreateSolidBrush(RGB(0x69, 0x69, 0x69));
        activatedKeyLedBrush = CreateSolidBrush(RGB(0x00, 0x80, 0x00));
    }
    ~KeyCodeBlocker()
    {
        if (labelFont)
        {
            DeleteObject(labelFont);
        }
        if (counterFont)
        {
            DeleteObject(counterFont);
        }
        if (lockNormalBrush)
        {
            DeleteObject(lockNormalBrush);
        }
        if (lockPressedBrush)
        {
            DeleteObject(lockPressedBrush);
        }
        if (unlockNormalBrush)
        {
            DeleteObject(unlockNormalBrush);
        }
        if (unlockPressedBrush)
        {
            DeleteObject(unlockPressedBrush);
        }
        if (activatedKeyLedBrush)
        {
            DeleteObject(activatedKeyLedBrush);
        }
    }

    void SetWnd(HWND hWnd)
    {
        this->hWnd = hWnd;
    }

    void Initialize()
    {
        struct KeyCode
        {
            int code;
            int group;
            int section;
            int rowSpan;
            float weight;
            float padding;
        };

        std::vector<std::vector<KeyCode>> keyCodeRows
        {
            std::vector<KeyCode> {
                {VK_ESCAPE, 0, 0, 1, 1.0f, 0.0f}, {NULL, 0, 0, 1, 1.0f, 0.0f}, {VK_F1, 1, 0, 1, 1.0f, 0.0f}, {VK_F2, 1, 0, 1, 1.0f, 0.0f}, {VK_F3, 1, 0, 1, 1.0f, 0.0f}, {VK_F4, 1, 0, 1, 1.0f, 0.0f}, {NULL, 1, 0, 1, 0.5f, -0.5f}, {VK_F5, 2, 0, 1, 1.0f, 0.0f}, {VK_F6, 2, 0, 1, 1.0f, 0.0f}, {VK_F7, 2, 0, 1, 1.0f, 0.0f}, {VK_F8, 2, 0, 1, 1.0f, 0.0f}, {NULL, 2, 0, 1, 0.5f, -0.5f}, {VK_F9, 3, 0, 1, 1.0f, 0.0f}, {VK_F10, 3, 0, 1, 1.0f, 0.0f}, {VK_F11, 3, 0, 1, 1.0f, 0.0f}, {VK_F12, 3, 0, 1, 1.0f, 0.0f},
                {VK_SNAPSHOT, 0, 1, 1, 1.0f, 0.0f}, {VK_SCROLL, 0, 1, 1, 1.0f, 0.0f}, {VK_PAUSE, 0, 1, 1, 1.0f, 0.0f},
            },
            std::vector<KeyCode> {
                {VK_OEM_3, 0, 0, 1, 1.0f, 0.0f}, {'1', 0, 0, 1, 1.0f, 0.0f}, {'2', 0, 0, 1, 1.0f, 0.0f}, {'3', 0, 0, 1, 1.0f, 0.0f}, {'4', 0, 0, 1, 1.0f, 0.0f}, {'5', 0, 0, 1, 1.0f, 0.0f}, {'6', 0, 0, 1, 1.0f, 0.0f}, {'7', 0, 0, 1, 1.0f, 0.0f}, {'8', 0, 0, 1, 1.0f, 0.0f}, {'9', 0, 0, 1, 1.0f, 0.0f}, {'0', 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_MINUS, 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_PLUS, 0, 0, 1, 1.0f, 0.0f}, {VK_BACK, 0, 0, 1, 2.0f, 1.0f},
                {VK_INSERT, 0, 1, 1, 1.0f, 0.0f}, {VK_HOME, 0, 1, 1, 1.0f, 0.0f}, {VK_PRIOR, 0, 1, 1, 1.0f, 0.0f},
                {VK_NUMLOCK, 0, 2, 1, 1.0f, 0.0f}, {VK_DIVIDE, 0, 2, 1, 1.0f, 0.0f}, {VK_MULTIPLY, 0, 2, 1, 1.0f, 0.0f}, {VK_SUBTRACT, 0, 2, 1, 1.0f, 0.0f},
            },
            std::vector<KeyCode> {
                {VK_TAB, 0, 0, 1, 1.5f, 0.5f}, {'Q', 0, 0, 1, 1.0f, 0.0f}, {'W', 0, 0, 1, 1.0f, 0.0f}, {'E', 0, 0, 1, 1.0f, 0.0f}, {'R', 0, 0, 1, 1.0f, 0.0f}, {'T', 0, 0, 1, 1.0f, 0.0f}, {'Y', 0, 0, 1, 1.0f, 0.0f}, {'U', 0, 0, 1, 1.0f, 0.0f}, {'I', 0, 0, 1, 1.0f, 0.0f}, {'O', 0, 0, 1, 1.0f, 0.0f}, {'P', 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_4, 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_6, 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_5, 0, 0, 1, 1.5f, 0.5f},
                {VK_DELETE, 0, 1, 1, 1.0f, 0.0f}, {VK_END, 0, 1, 1, 1.0f, 0.0f}, {VK_NEXT, 0, 1, 1, 1.0f, 0.0f},
                {VK_NUMPAD7, 0, 2, 1, 1.0f, 0.0f}, {VK_NUMPAD8, 0, 2, 1, 1.0f, 0.0f}, {VK_NUMPAD9, 0, 2, 1, 1.0f, 0.0f}, {VK_ADD, 0, 2, 2, 1.0f, 0.0f},
            },
            std::vector<KeyCode> {
                {VK_CAPITAL, 0, 0, 1, 1.8f, 1.0f}, {'A', 0, 0, 1, 1.0f, 0.0f}, {'S', 0, 0, 1, 1.0f, 0.0f}, {'D', 0, 0, 1, 1.0f, 0.0f}, {'F', 0, 0, 1, 1.0f, 0.0f}, {'G', 0, 0, 1, 1.0f, 0.0f}, {'H', 0, 0, 1, 1.0f, 0.0f}, {'J', 0, 0, 1, 1.0f, 0.0f}, {'K', 0, 0, 1, 1.0f, 0.0f}, {'L', 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_1, 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_7, 0, 0, 1, 1.0f, 0.0f}, {VK_RETURN, 0, 0, 1, 2.2f, 1.0f},
                {VK_NUMPAD4, 0, 2, 1, 1.0f, 0.0f}, {VK_NUMPAD5, 0, 2, 1, 1.0f, 0.0f}, {VK_NUMPAD6, 0, 2, 1, 1.0f, 0.0f},
            },
            std::vector<KeyCode> {
                {VK_LSHIFT, 0, 0, 1, 2.5f, 1.5f}, {'Z', 0, 0, 1, 1.0f, 0.0f}, {'X', 0, 0, 1, 1.0f, 0.0f}, {'C', 0, 0, 1, 1.0f, 0.0f}, {'V', 0, 0, 1, 1.0f, 0.0f}, {'B', 0, 0, 1, 1.0f, 0.0f}, {'N', 0, 0, 1, 1.0f, 0.0f}, {'M', 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_COMMA, 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_PERIOD, 0, 0, 1, 1.0f, 0.0f}, {VK_OEM_2, 0, 0, 1, 1.0f, 0.0f}, {VK_RSHIFT, 0, 0, 1, 2.5f, 1.5f},
                {NULL, 0, 1, 1, 1.0f, 0.0f}, {VK_UP, 0, 1, 1, 1.0f, 0.0f}, {NULL, 0, 1, 1, 1.0f, 0.0f},
                {VK_NUMPAD1, 0, 2, 1, 1.0f, 0.0f}, {VK_NUMPAD2, 0, 2, 1, 1.0f, 0.0f}, {VK_NUMPAD3, 0, 2, 1, 1.0f, 0.0f}, {VK_RETURN, 0, 2, 2, 1.0f, 0.0f},
            },
            std::vector<KeyCode> {
                {VK_LCONTROL, 0, 0, 1, 1.2f, 1.0f}, {VK_LWIN, 0, 0, 1, 1.2f, 1.0f}, {VK_LMENU, 0, 0, 1, 1.2f, 1.0f}, {VK_SPACE, 0, 0, 1, 7.8f, 2.0f}, {VK_HANGUL, 0, 0, 1, 1.2f, 1.0f}, {VK_RWIN, 0, 0, 1, 1.2f, 1.0f}, {VK_HANJA, 0, 0, 1, 1.2f, 1.0f},
                {VK_LEFT, 0, 1, 1, 1.0f, 0.0f}, {VK_DOWN, 0, 1, 1, 1.0f, 0.0f}, {VK_RIGHT, 0, 1, 1, 1.0f, 0.0f},
                {VK_NUMPAD0, 0, 2, 1, 2.0f, 1.0f}, {VK_DECIMAL, 0, 2, 1, 1.0f, 0.0f},
            },
        };

        keyCapRows.resize(keyCodeRows.size());
        for (size_t row = 0; row < keyCapRows.size(); row++)
        {
            int column = 0;
            float offset = 0.0f;
            float paddingSum = 0.0f;
            int section = 0;

            std::vector<KeyCap>& keyCaps = keyCapRows[row];
            std::vector<KeyCode>& keyCodes = keyCodeRows[row];

            keyCaps.resize(keyCodes.size());

            for (size_t col = 0; col < keyCodes.size(); col++)
            {
                const KeyCode& keyCode = keyCodes[col];

                if (section != keyCode.section)
                {
                    column = 0;
                    offset = 0.0f;
                    paddingSum = 0.0f;
                    section = keyCode.section;
                }

                if (keyCode.code != 0)
                {
                    KeyCap& keyCap = keyCaps[col];

                    keyCap.row = int(row);
                    keyCap.rowSpan = keyCode.rowSpan;
                    keyCap.group = keyCode.group;
                    keyCap.column = column;
                    keyCap.offset = offset;
                    keyCap.section = keyCode.section;
                    keyCap.keyCode = keyCode.code;
                    keyCap.weight = keyCode.weight;
                    keyCap.padding = keyCode.padding;
                    keyCap.paddingSum = paddingSum;
                }

                column++;
                offset += keyCode.weight;
                paddingSum += keyCode.padding;
            }
        }
    }

    void Toggle(const int x, const int y)
    {
        for (size_t row = 0; row < keyCapRows.size(); row++)
        {
            std::vector<KeyCap>& keyCaps = keyCapRows[row];

            for (size_t col = 0; col < keyCaps.size(); col++)
            {
                RECT& rc = keyCaps[col].rect;

                if (x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom)
                {
                    lock[keyCaps[col].keyCode] = !lock[keyCaps[col].keyCode];

                    SendMessage(hWnd, WM_USER + 1, 0, 0);
                    return;
                }
            }
        }
    }

    void Resize(int width, int height)
    {
        this->width = width;
        this->height = height;
    }

    void Paint(HWND hWnd)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        Draw(hdc);
        EndPaint(hWnd, &ps);
    }

    void Draw(HWND hWnd)
    {
        HDC hdc = GetDC(hWnd);
        Draw(hdc);
        ReleaseDC(hWnd, hdc);
    }

    void Start()
    {
        HookStart(this);
    }

    void Stop()
    {
        HookStop();
    }

private:
    void Draw(HDC hdc)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ defaultBitmap = SelectObject(memDC, memBitmap);
        HGDIOBJ defaultBrush = GetCurrentObject(memDC, OBJ_BRUSH);

        SetBkMode(memDC, TRANSPARENT);
        PatBlt(memDC, 0, 0, width, height, BLACKNESS);

        HFONT defaultFont = HFONT(GetCurrentObject(memDC, OBJ_FONT));
        HFONT labelFont = this->labelFont ? this->labelFont : defaultFont;
        HFONT counterFont = this->counterFont ? this->counterFont : defaultFont;

        // total column 22
        // group 0 column 15
        // group 1 column 3
        // group 2 column 4
        float margin = 5;
        float marginGroup = 15;
        float padding = 2;
        float rowCapsWidth = width - (margin * 2.0f) - (marginGroup * 2.0f) - (padding * 19.0f);

        float keyCapWidth = rowCapsWidth / 22.0f;
        float keyCapHeight = keyCapWidth;

        float groupOffsets[3] =
        {
            margin,
            // 키간의 padding만 계산하기위해 15 - 1
            margin + (keyCapWidth * 15) + (padding * 14) + marginGroup,
            // 키간의 padding만 계산하기위해 (15 - 1) + (3 - 1)
            margin + (keyCapWidth * 18) + (padding * 16) + marginGroup * 2,
        };

        bool activatedCapsLock = (GetKeyState(VK_CAPITAL) & 0x01) == 0x01;
        bool activatedNumLock = (GetKeyState(VK_NUMLOCK) & 0x01) == 0x01;
        bool activatedScrLock = (GetKeyState(VK_SCROLL) & 0x01) == 0x01;

        float offsetY = margin;
        for (int row = 0; row < keyCapRows.size(); row++)
        {
            std::vector<KeyCap>& keyCaps = keyCapRows[row];

            for (int col = 0; col < keyCaps.size(); col++)
            {
                KeyCap& keyCap = keyCaps[col];

                if (keyCap.keyCode == 0)
                {
                    continue;
                }
                const TCHAR* label = KeyCapLabels[keyCap.keyCode];

                float offsetX = groupOffsets[keyCap.section] + (keyCapWidth * keyCap.offset) + (padding * keyCap.column) + (padding * keyCap.paddingSum);

                float x1 = 0.0f;
                float y1 = 0.0f;
                float x2 = keyCapWidth * keyCap.weight + padding * keyCap.padding;
                float y2 = keyCapHeight * keyCap.rowSpan + padding * (keyCap.rowSpan - 1);

                x1 += offsetX;
                x2 += offsetX;
                y1 += offsetY;
                y2 += offsetY;

                RECT rc =
                {
                    static_cast<LONG>(x1),
                    static_cast<LONG>(y1),
                    static_cast<LONG>(x2),
                    static_cast<LONG>(y2)
                };
                RECT rcCap =
                {
                    rc.left + 2,
                    rc.top + 2,
                    rc.right - 2,
                    rc.bottom - 2
                };

                TCHAR text[64];
                _stprintf_s(text, TEXT("%lld"), counter[keyCap.keyCode]);

                HBRUSH background = lock[keyCap.keyCode]
                    ? (press[keyCap.keyCode] ? lockPressedBrush : lockNormalBrush)
                    : (press[keyCap.keyCode] ? unlockPressedBrush : unlockNormalBrush);
                FillRect(memDC, &rc, background);

                SelectObject(memDC, counterFont);
                DrawText(memDC, text, int(_tcslen(text)), &rcCap, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);

                SelectObject(memDC, labelFont);
                DrawText(memDC, label, int(_tcslen(label)), &rcCap, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                switch (keyCap.keyCode)
                {
                case VK_CAPITAL:
                    if (activatedCapsLock)
                    {
                        SelectObject(memDC, activatedKeyLedBrush);
                        Ellipse(memDC, rcCap.right - 5, rcCap.top, rcCap.right, rcCap.top + 5);
                    }
                    break;
                case VK_NUMLOCK:
                    if (activatedNumLock)
                    {
                        SelectObject(memDC, activatedKeyLedBrush);
                        Ellipse(memDC, rcCap.right - 5, rcCap.top, rcCap.right, rcCap.top + 5);
                    }
                    break;
                case VK_SCROLL:
                    if (activatedScrLock)
                    {
                        SelectObject(memDC, activatedKeyLedBrush);
                        Ellipse(memDC, rcCap.right - 5, rcCap.top, rcCap.right, rcCap.top + 5);
                    }
                    break;
                }

                keyCaps[col].rect = rc;
            }
            if (row == 0)
            {
                offsetY += marginGroup;
            }
            offsetY += keyCapHeight + padding;
        }
        if (defaultFont)
        {
            SelectObject(memDC, defaultFont);
        }

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, defaultBitmap);
        SelectObject(memDC, defaultBrush);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
    }

    LRESULT OnHookProc(int nCode, WPARAM wParam, LPARAM lParam) override
    {
        DWORD vkCode = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam)->vkCode;

        if (vkCode < 256)
        {
            if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && press[vkCode] == false)
            {
                press[vkCode] = true;
                counter[vkCode]++;

                //TCHAR message[128];
                //_stprintf_s(message, TEXT("%04llx: 0x%02x (%d)\n"), wParam, vkCode, vkCode);
                //OutputDebugString(message);

                PostMessage(hWnd, WM_USER + 1, 0, 0);
            }
            if ((wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
            {
                press[vkCode] = false;

                PostMessage(hWnd, WM_USER + 1, 0, 0);
            }
            if (lock[vkCode])
            {
                return TRUE;
            }
        }
        return FALSE;
    }

    int width;
    int height;
    bool lock[256];
    bool press[256];
    long long counter[256];
    std::vector<std::vector<KeyCap>> keyCapRows;

    HFONT labelFont;
    HFONT counterFont;

    HBRUSH lockNormalBrush;
    HBRUSH lockPressedBrush;
    HBRUSH unlockNormalBrush;
    HBRUSH unlockPressedBrush;
    HBRUSH activatedKeyLedBrush;

    HWND hWnd;
};

int APIENTRY _tWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = HINST_THISCOMPONENT;
    //wcex.hIcon = LoadIcon(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDI_APPICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = HBRUSH(GetStockObject(BLACK_BRUSH));
    wcex.lpszClassName = szWindowClass;
    //wcex.hIconSm = LoadIcon(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_MAINMENU));

    if (RegisterClassEx(&wcex) == 0)
    {
        return EXIT_FAILURE;
    }

    KeyCodeBlocker blocker;

    RECT rc = { 0, 0, clientWidth, clientHeight };
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rc, dwStyle, FALSE);

    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL,
        HINST_THISCOMPONENT, &blocker);

    if (hWnd == NULL)
    {
        return EXIT_FAILURE;
    }
    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);

    MSG message = {};
    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return static_cast<int>(message.wParam);
}

LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    KeyCodeBlocker* blocker = reinterpret_cast<KeyCodeBlocker*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (blocker)
    {
        switch (message)
        {
        case WM_CREATE:
            blocker->Initialize();
            blocker->SetWnd(hWnd);
            blocker->Start();
            return 0;
        case WM_PAINT:
            blocker->Paint(hWnd);
            return 0;
        case WM_USER + 1:
            blocker->Draw(hWnd);
            return 0;
        case WM_RBUTTONUP:
            blocker->Toggle(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_SIZE:
            blocker->Resize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_CLOSE:
            blocker->Stop();
            blocker->SetWnd(NULL);
            DestroyWindow(hWnd);
            return 0;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
