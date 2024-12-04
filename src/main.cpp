// This file contains a substantial portion of MIT licensed Microsoft code:
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
// Copyright (c) Microsoft Corporation. All rights reserved
//
// This derivative is sublicensed as Mozilla Public License Version 2.0

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <strsafe.h>
#include "resource.h"
#include "hid.h"
#include <initguid.h>

static HINSTANCE g_hInst = nullptr;

static UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;

static wchar_t const szWindowClass[] = L"NotificationIconTest";

// Use a guid to uniquely identify our icon "9D0B8B92-4E1C-488e-A1E1-2331AFCE2CB5"
DEFINE_GUID(GUID_PrinterIcon, 0x9D0B8B92, 0x4E1C, 0x488e, 0xA1, 0xE1, 0x23, 0x31, 0xAF, 0xCE, 0x2C, 0xB5);

// Forward declarations of functions included in this code module:
void                RegisterWindowClass(PCWSTR pszClassName, PCWSTR pszMenuName, WNDPROC lpfnWndProc);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL                AddNotificationIcon(HWND hwnd);
BOOL                DeleteNotificationIcon();

#define BRIGHTNESS_STEP 5000
#define BRIGHTNESS_MIN 400
#define BRIGHTNESS_MAX 60000
#define HOLD_KEY_1 VK_LSHIFT
#define HOLD_KEY_2 VK_LWIN
#define DOWN_KEY VK_LEFT
#define UP_KEY VK_RIGHT

static HHOOK hook = nullptr;
static bool holdKey1Down = false;
static bool holdKey2Down = false;
static ULONG currentBrightness = 30000;

static void onStepDown () {
  if (currentBrightness > BRIGHTNESS_MIN + BRIGHTNESS_STEP) {
    currentBrightness -= BRIGHTNESS_STEP;
  } else {
    currentBrightness = BRIGHTNESS_MIN;
  }

  int err = hid_setBrightness(currentBrightness);
  if (err < 0) {
    char errStr[100];
    snprintf(errStr, sizeof(errStr), "hid_setBrightness returned %d\n", err);
    MessageBoxA(nullptr, errStr, "studio-brightness", MB_ICONINFORMATION);
    currentBrightness = 30000;
  }
}

static void onStepUp () {
  if (currentBrightness < BRIGHTNESS_MAX - BRIGHTNESS_STEP) {
    currentBrightness += BRIGHTNESS_STEP;
  } else {
    currentBrightness = BRIGHTNESS_MAX;
  }

  int err = hid_setBrightness(currentBrightness);
  if (err < 0) {
    char errStr[100];
    snprintf(errStr, sizeof(errStr), "hid_setBrightness returned %d\n", err);
    MessageBoxA(nullptr, errStr, "studio-brightness", MB_ICONINFORMATION);
    currentBrightness = 30000;
  }
}

LRESULT hookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode < 0) return CallNextHookEx(hook, nCode, wParam, lParam);

  KBDLLHOOKSTRUCT kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
  if (wParam == WM_KEYDOWN) {
    if (kbdStruct.vkCode == HOLD_KEY_1) {
      holdKey1Down = true;
    } else if (kbdStruct.vkCode == HOLD_KEY_2) {
      holdKey2Down = true;
    } else if (kbdStruct.vkCode == DOWN_KEY && holdKey1Down && holdKey2Down) {
      onStepDown();
    } else if (kbdStruct.vkCode == UP_KEY && holdKey1Down && holdKey2Down) {
      onStepUp();
    }
  } else if (wParam == WM_KEYUP) {
    if (kbdStruct.vkCode == HOLD_KEY_1) {
      holdKey1Down = false;
    } else if (kbdStruct.vkCode == HOLD_KEY_2) {
      holdKey2Down = false;
    }
  }

  return CallNextHookEx(hook, nCode, wParam, lParam);
}

int initKeyboardHook () {
  hook = SetWindowsHookExW(WH_KEYBOARD_LL, hookCallback, nullptr, 0);
  return (hook) ? 0 : -1;
}

void deinitKeyboardHook () {
  if (hook) {
    UnhookWindowsHookEx(hook);
    hook = nullptr;
  }
}

void RegisterWindowClass(PCWSTR pszClassName, PCWSTR pszMenuName, WNDPROC lpfnWndProc)
{
    WNDCLASSEXW wcex = {sizeof(wcex)};
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = lpfnWndProc;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = pszMenuName;
    wcex.lpszClassName  = pszClassName;
    RegisterClassExW(&wcex);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, [[maybe_unused]] PWSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
    g_hInst = hInstance;
    RegisterWindowClass(szWindowClass, MAKEINTRESOURCEW(IDC_NOTIFICATIONICON), WndProc);

    // Create the main window. This could be a hidden window if you don't need
    // any UI other than the notification icon.
    WCHAR szTitle[100];
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));
    HWND hwnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 250, 200, nullptr, nullptr, g_hInst, nullptr);

    char errStr[100];

    int err = hid_init();
    if (err < 0) {
      snprintf(errStr, sizeof(errStr), "hid_init returned %d\n", err);
      MessageBoxA(nullptr, errStr, "studio-brightness", MB_ICONINFORMATION);
    }

    err = hid_getBrightness(&currentBrightness);
    if (err < 0) {
      snprintf(errStr, sizeof(errStr), "hid_getBrightness returned %d\n", err);
      MessageBoxA(nullptr, errStr, "studio-brightness", MB_ICONINFORMATION);
      currentBrightness = 30000;
    }

    err = initKeyboardHook();
    if (err < 0) {
      snprintf(errStr, sizeof(errStr), "initKeyboardHook returned %d\n", err);
      MessageBoxA(nullptr, errStr, "studio-brightness", MB_ICONINFORMATION);
    }

    if (hwnd)
    {
        // Main message loop:
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

BOOL AddNotificationIcon(HWND hwnd)
{
    NOTIFYICONDATA nid = {sizeof(nid)};
    nid.hWnd = hwnd;
    // add the icon, setting the icon, tooltip, and callback message.
    // the icon will be identified with the GUID
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
    nid.guidItem = GUID_PrinterIcon;
    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    nid.hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    LoadString(g_hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
    Shell_NotifyIcon(NIM_ADD, &nid);

    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL DeleteNotificationIcon()
{
    NOTIFYICONDATA nid = {sizeof(nid)};
    nid.uFlags = NIF_GUID;
    nid.guidItem = GUID_PrinterIcon;
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // add the notification icon
        if (!AddNotificationIcon(hwnd))
        {
            MessageBoxW(hwnd,
                L"Please read the ReadMe.txt file for troubleshooting",
                L"Error adding icon", MB_OK);
            return -1;
        }
        break;
    case WM_COMMAND:
        {
            int const wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_OPTIONS:
                // placeholder for an options dialog
                MessageBoxW(hwnd,  L"Display the options dialog here.", L"Options", MB_OK);
                break;

            case IDM_EXIT:
                DestroyWindow(hwnd);
                break;

            default:
                return DefWindowProc(hwnd, message, wParam, lParam);
            }
        }
        break;

    case WMAPP_NOTIFYCALLBACK:
        switch (LOWORD(lParam)) {
          case NIN_SELECT:
          case WM_CONTEXTMENU:
          {
              POINT pt;
              GetCursorPos(&pt);

              HMENU hmenu = CreatePopupMenu();
              InsertMenuW(hmenu, 0, MF_BYPOSITION | MF_STRING, 100, L"Exit");

              SetForegroundWindow(hwnd);

              int cmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
              if (cmd != 0) DestroyWindow(hwnd);
          }
          break;
        }
        break;

    case WM_DESTROY:
        deinitKeyboardHook();
        hid_deinit();
        DeleteNotificationIcon();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
