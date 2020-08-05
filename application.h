#pragma once

#include <Windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <memory>

#include "item.h"

class Application {
public:
    HINSTANCE m_hInstance;
    HWND m_mainWindow;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void GitStatus();
    void GitBranch();

    void OnCreate(HWND hwnd);
    void OnPaint(HWND hwnd);
    void OnSize(HWND hwnd);
    void OnChar(WPARAM wparam);
    void OnKeyDown(HWND hwnd, WPARAM wparam);
    void paint(Gdiplus::Graphics & graphics);
    void layout(const Gdiplus::RectF & rect);
    void SendToTerminal(const std::wstring & str);
    void UpdateFilter();
    bool SelectNextMatch();
    bool SelectPrevMatch();
    void ScrollIntoView();

    Gdiplus::Bitmap * bitmap = NULL;
    std::vector<Item> m_items;
    Gdiplus::RectF itemsRect;
    Gdiplus::RectF statusbarRect;
    Gdiplus::RectF scrollbarRect;
    int scrollOffset = 0;
    static constexpr float ITEM_HEIGHT = 13.0f;

    std::unique_ptr< Gdiplus::FontFamily > fontFamily;
    std::unique_ptr< Gdiplus::Font > font;
    std::unique_ptr< Gdiplus::FontFamily > nmfontFamily;
    std::unique_ptr< Gdiplus::Font > nmfont;
    std::wstring m_filterString;
    std::wstring m_root;
    std::wstring m_git = L"C:\\Program Files\\Git\\cmd\\git.exe";
    std::wstring m_sublime = L"C:\\Program Files\\Sublime Text 3\\sublime_text.exe";

    static constexpr int DEFAULT_PAGE_SIZE = 20;

    int currentItem = 0;
    bool m_searchMode = false;
    int m_pagesize = DEFAULT_PAGE_SIZE;
    std::wstring m_message;
    enum mode_t {
        STATUS,
        BRANCH
    } m_mode = STATUS;
};

extern Application g_app;
