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
#include <format>
#include <cstdlib>

#define DEBUG_MESSAGES 0

static HINSTANCE g_hInst = nullptr;

constexpr UINT WMAPP_NOTIFYCALLBACK = WM_APP + 1;

constexpr wchar_t szWindowClass[] = L"NotificationIconTest";

// Use a guid to uniquely identify our icon "9D0B8B92-4E1C-488e-A1E1-2331AFCE2CB5"
DEFINE_GUID(GUID_PrinterIcon, 0x9D0B8B92, 0x4E1C, 0x488e, 0xA1, 0xE1, 0x23, 0x31, 0xAF, 0xCE, 0x2C, 0xB5);

// Forward declarations of functions included in this code module:
void                RegisterWindowClass(PCWSTR pszClassName, PCWSTR pszMenuName, WNDPROC lpfnWndProc);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL                AddNotificationIcon(HWND hwnd);
BOOL                DeleteNotificationIcon();

constexpr ULONG BRIGHTNESS_STEPS[15] = { 400, 2400, 4400, 7200, 10000, 15000, 20000, 25000, 30000, 35000, 40000, 45000, 50000, 55000, 60000 };
constexpr DWORD HOLD_KEY_1 = VK_LSHIFT;
constexpr DWORD HOLD_KEY_2 = VK_LWIN;
constexpr DWORD DOWN_KEY = VK_LEFT;
constexpr DWORD UP_KEY = VK_RIGHT;

static HHOOK hook = nullptr;
static bool holdKey1Down = false;
static bool holdKey2Down = false;
static ULONG currentBrightnessIndex = 8;
static ULONG currentBrightness = 30000;

static void onStepDown () {
  if (currentBrightnessIndex > 0) {
    currentBrightness = BRIGHTNESS_STEPS[--currentBrightnessIndex];
  }

  int err = hid_setBrightness(currentBrightness);
  if (err < 0) {
    #if DEBUG_MESSAGES
    MessageBoxA(nullptr, std::format("hid_setBrightness returned {}\n", err).data(), "studio-brightness", MB_ICONERROR);
    #endif
    currentBrightnessIndex = 8;
    currentBrightness = 30000;
  }
}

static void onStepUp () {
  if (currentBrightnessIndex < 14) {
    currentBrightness = BRIGHTNESS_STEPS[++currentBrightnessIndex];
  }

  int err = hid_setBrightness(currentBrightness);
  if (err < 0) {
    #if DEBUG_MESSAGES
    MessageBoxA(nullptr, std::format("hid_setBrightness returned {}\n", err).data(), "studio-brightness", MB_ICONERROR);
    #endif
    currentBrightnessIndex = 8;
    currentBrightness = 30000;
  }
}

LRESULT __stdcall hookCallback (int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode < 0) return CallNextHookEx(hook, nCode, wParam, lParam);

  KBDLLHOOKSTRUCT kbdStruct = *reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
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

void RegisterWindowClass (PCWSTR pszClassName, PCWSTR pszMenuName, WNDPROC lpfnWndProc) {
  WNDCLASSEXW wcex = {sizeof(wcex)};
  wcex.style          = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc    = lpfnWndProc;
  wcex.hInstance      = g_hInst;
  wcex.hIcon          = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON));
  wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
  wcex.lpszMenuName   = pszMenuName;
  wcex.lpszClassName  = pszClassName;
  RegisterClassExW(&wcex);
}

int APIENTRY wWinMain (HINSTANCE hInstance, HINSTANCE, [[maybe_unused]] PWSTR lpCmdLine, [[maybe_unused]] int nCmdShow) {
  g_hInst = hInstance;
  RegisterWindowClass(szWindowClass, MAKEINTRESOURCEW(IDC_NOTIFICATIONICON), WndProc);

  // Create the main window. This could be a hidden window if you don't need
  // any UI other than the notification icon.
  WCHAR szTitle[100];
  LoadStringW(hInstance, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));
  HWND hwnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, 0, 250, 200, nullptr, nullptr, g_hInst, nullptr);

  int err = hid_init();
  if (err < 0) {
    #if DEBUG_MESSAGES
    MessageBoxA(nullptr, std::format("hid_init returned {}", err).data(), "studio-brightness", MB_ICONERROR);
    #endif
    return EXIT_FAILURE;
  }

    err = hid_getBrightness(&currentBrightness);

    // find the BRIGHTNESS_STEPS[currentBrightnessIndex] nearest to currentBrightness
    for (currentBrightnessIndex = 0; currentBrightnessIndex < 15; currentBrightnessIndex++) {
      if (BRIGHTNESS_STEPS[currentBrightnessIndex] > currentBrightness) break;
    }

    if (err < 0) {
      #if DEBUG_MESSAGES
      MessageBoxA(nullptr, std::format("hid_getBrightness returned {}", err).data(), "studio-brightness", MB_ICONERROR);
      #endif
      currentBrightnessIndex = 8;
      currentBrightness = 30000;
    }

  err = initKeyboardHook();
  if (err < 0) {
    #if DEBUG_MESSAGES
    MessageBoxA(nullptr, std::format("initKeyboardhook returned {}", err).data(), "studio-brightness", MB_ICONERROR);
    #endif
    return EXIT_FAILURE;
  }

  if (hwnd) {
    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return EXIT_SUCCESS;
}

BOOL AddNotificationIcon (HWND hwnd) {
  NOTIFYICONDATA nid = {sizeof(nid)};
  nid.hWnd = hwnd;
  // add the icon, setting the icon, tooltip, and callback message.
  // the icon will be identified with the GUID
  nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
  nid.guidItem = GUID_PrinterIcon;
  nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
  nid.hIcon = static_cast<HICON>(LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
  LoadString(g_hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
  Shell_NotifyIcon(NIM_ADD, &nid);

  // NOTIFYICON_VERSION_4 is prefered
  nid.uVersion = NOTIFYICON_VERSION_4;
  return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL DeleteNotificationIcon ()
{
    NOTIFYICONDATA nid = {sizeof(nid)};
    nid.uFlags = NIF_GUID;
    nid.guidItem = GUID_PrinterIcon;
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
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
      break;
    }

    case WM_MEASUREITEM:
    {
      LPMEASUREITEMSTRUCT pmis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
      pmis->itemWidth = 250; // Specify width
      pmis->itemHeight = 25; // Specify height
      return TRUE;
    }

    case WM_DRAWITEM:
    {
      LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
      HDC hdc = pdis->hDC;
      RECT rect = pdis->rcItem;

      // Draw menu text and shortcut aligned
      if (pdis->itemID == 100) {
        RECT shortcutRect = rect;
        shortcutRect.left += 20;
        DrawTextW(hdc, L"Exit", -1, &shortcutRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      } else if (pdis->itemID == 101) {
        RECT shortcutRect = rect;
        shortcutRect.left += 20;
        DrawTextW(hdc, L"Increase Brightness", -1, &shortcutRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        shortcutRect.left += 120;
        DrawTextW(hdc, L"LShift+LWin+Right", -1, &shortcutRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      } else if (pdis->itemID == 102) {
        RECT shortcutRect = rect;
        shortcutRect.left += 20;
        DrawTextW(hdc, L"Decrease Brightness", -1, &shortcutRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        shortcutRect.left += 120;
        DrawTextW(hdc, L"LShift+LWin+Left", -1, &shortcutRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      }
      return TRUE;
    }

    case WMAPP_NOTIFYCALLBACK:
      switch (LOWORD(lParam)) {
        case NIN_SELECT:
        case WM_CONTEXTMENU:
        {
          POINT pt;
          GetCursorPos(&pt);

          HMENU hmenu = CreatePopupMenu();
          InsertMenuW(hmenu, 0, MF_BYPOSITION | MF_OWNERDRAW, 100, L"Exit");
          InsertMenuW(hmenu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
          InsertMenuW(hmenu, 0, MF_BYPOSITION | MF_OWNERDRAW, 102, L"Decrease Brightness");
          InsertMenuW(hmenu, 0, MF_BYPOSITION | MF_OWNERDRAW, 101, L"Increase Brightness");

          SetForegroundWindow(hwnd);

          int cmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
          switch (cmd) {
            case 100:
            {
              DestroyWindow(hwnd);
              break;
            }

            case 101:
            {
              onStepUp();
              break;
            }

            case 102:
            {
              onStepDown();
              break;
            }
          }
          break;
        }
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
