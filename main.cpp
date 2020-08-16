#include <Windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <array>
#include <CommCtrl.h>
#include <gdiplus.h>

#include "application.h"
#include "visualstudio.h"

using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

int wmain(int argc, wchar_t * argv[], wchar_t * envp[]) {
    HWND currhwnd = ::FindWindow(L"WindowsTerminalGitTool", NULL);
    if (currhwnd != NULL) {
        SwitchToThisWindow(currhwnd, TRUE);
        return 0;
    }

    if (argc < 2) {
        return 0;
    }

    if (OleInitialize(NULL) != S_OK) {
        return 0;
    }

    g_app.m_hInstance = GetModuleHandle(NULL);

    if (argc > 1) {
        if (argv[1] == std::wstring(L"-b")) {
            g_app.m_viewStack[0] = {Application::BRANCH, 0, L"", L""};
            if (argc > 2) {
                g_app.m_root = argv[2];
            }
        } else {
            g_app.m_root = argv[1];
        }
    }

    WNDCLASS wClass{};
    wClass.lpszClassName = L"WindowsTerminalGitTool";
    wClass.hInstance = g_app.m_hInstance;
    wClass.lpfnWndProc = Application::WndProc;
    wClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (RegisterClass(&wClass) == 0) {
        MessageBox(0, L"RegisterClass() failed", 0, MB_OK);
        return 0;
    }
    
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // determine window position and size
    int winx = CW_USEDEFAULT;
    int winy = CW_USEDEFAULT;
    int winwidth = CW_USEDEFAULT;
    int winheight = CW_USEDEFAULT;
    HWND hwndTerminal = FindWindow(L"CASCADIA_HOSTING_WINDOW_CLASS", NULL);
    if (hwndTerminal != NULL) {
        RECT rect;
        GetWindowRect(hwndTerminal, &rect);
        winx = rect.left;
        winy = rect.top;
        winwidth = rect.right - rect.left;
        winheight = rect.bottom - rect.top;
    }

    HWND hWnd = CreateWindowEx(0, L"WindowsTerminalGitTool", L"WindowsTerminalGitTool", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, winx, winy, winwidth, winheight, NULL, NULL, g_app.m_hInstance, NULL);
    if (hWnd == NULL) {
        MessageBox(0, L"CreateWindow failed", 0, MB_OK);
        return 0;
    }

    g_app.Refresh();

    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);
    
    MSG msg;
    while (BOOL bRet = GetMessage(&msg, NULL, 0, 0) != 0) {
        if (bRet == -1) {
            return 0;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    OleUninitialize();
    return 0;
}
