#include "pch.h"
#include "mousehook.h"
#include <stdio.h>

static unsigned long    g_ulCaughtCount = 0;
static int              g_nMonitorButtons = 0;
static int              g_nMouseDownTimeoutMs = 0;
static int              g_nMouseUpTimeoutMs = 0;
int                     g_nCurrentTimeoutMs = 0;
static int              g_nRegisterRelease = 0;
static DWORD            g_dwLMBPreviousTicks = 0;
static DWORD            g_dwRMBPreviousTicks = 0;
static DWORD            g_dwMMBPreviousTicks = 0;

HHOOK hkKey = NULL;
HINSTANCE hInstHookDll = NULL;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID lpReserved) {
    switch (reasonForCall) {
        case DLL_PROCESS_ATTACH:
            hInstHookDll = (HINSTANCE)hModule;
            break;
    }
    return TRUE;
}

void __declspec(dllexport) __cdecl SetMouseHook(INT nMonitorButtons, INT nMouseDownTimeoutMs, INT nMouseUpTimeoutMs, INT nRegisterRelease) {
    if (hkKey == NULL)
        hkKey = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstHookDll, 0);

    g_nMonitorButtons = nMonitorButtons;
    g_nMouseDownTimeoutMs = nMouseDownTimeoutMs;
    g_nMouseUpTimeoutMs = nMouseUpTimeoutMs;
    g_nRegisterRelease = nRegisterRelease;
}

void __declspec(dllexport) __cdecl RemoveMouseHook(void) {
    if (hkKey != NULL)
        UnhookWindowsHookEx(hkKey);

    hkKey = NULL;

    g_ulCaughtCount = 0;
}

unsigned long __declspec(dllexport) __cdecl GetMouseHookCaughtCount(void) {
    return g_ulCaughtCount;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {


    #pragma region ButtonDown
        if (wParam == WM_LBUTTONDOWN && (g_nMonitorButtons & MHK_LEFT_MOUSE_BUTTON)) {
            if (CheckTicks(&g_dwLMBPreviousTicks, 1)) {
                // Ignore the click if it falls within the timeout period
                return 1;
            } else g_nCurrentTimeoutMs = g_nMouseDownTimeoutMs;
        }
        if (wParam == WM_RBUTTONDOWN && (g_nMonitorButtons & MHK_RIGHT_MOUSE_BUTTON)){
            if (CheckTicks(&g_dwRMBPreviousTicks, 1)) {
                return 1;
            }
            else g_nCurrentTimeoutMs = g_nMouseDownTimeoutMs;
        }
        if (wParam == WM_MBUTTONDOWN && (g_nMonitorButtons & MHK_MIDDLE_MOUSE_BUTTON)) {
            if (CheckTicks(&g_dwMMBPreviousTicks, 1)) {
                return 1;
            }
            else g_nCurrentTimeoutMs = g_nMouseDownTimeoutMs;
        }
    #pragma endregion

    #pragma region ButtonUp
        if (g_nRegisterRelease) {
            if (wParam == WM_LBUTTONUP && (g_nMonitorButtons & MHK_LEFT_MOUSE_BUTTON)) {
                g_nCurrentTimeoutMs = g_nMouseUpTimeoutMs;
                SetTicks(&g_dwLMBPreviousTicks);
            }
            if (wParam == WM_RBUTTONUP && (g_nMonitorButtons & MHK_RIGHT_MOUSE_BUTTON)) {
                g_nCurrentTimeoutMs = g_nMouseUpTimeoutMs;
                SetTicks(&g_dwRMBPreviousTicks);
            }
            if (wParam == WM_MBUTTONUP && (g_nMonitorButtons & MHK_MIDDLE_MOUSE_BUTTON)) {
                g_nCurrentTimeoutMs = g_nMouseUpTimeoutMs;
                SetTicks(&g_dwMMBPreviousTicks);
            }
        }
    #pragma endregion
    }

    return CallNextHookEx(hkKey, nCode, wParam, lParam);
}

BOOL CheckTicks(DWORD *dwPreviousTicks, int inputDown) {
    DWORD tickDelta = SetTicks(dwPreviousTicks);

    //printf("Tick Delta: %lu\n", g_nCurrentTimeoutMs);
    if (tickDelta <= (DWORD)g_nCurrentTimeoutMs) {
        g_ulCaughtCount++;
        return TRUE;
    }
    else {
        return FALSE;
    } 
}

DWORD SetTicks(DWORD* dwPreviousTicks) {
    DWORD dwTicks = GetTickCount();
    DWORD dwTickDelta = dwTicks - *dwPreviousTicks;

    *dwPreviousTicks = dwTicks;

    return dwTickDelta;
}