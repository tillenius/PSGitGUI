#define NOMINMAX

#include <algorithm>

namespace Gdiplus
{
using std::min;
using std::max;
}

#include "application.h"
#include "runscript.h"
#include "visualstudio.h"

#include "Shlobj.h"
#include <iostream>
#include <sstream>

Application g_app;

namespace {

enum { STATUS_NAME = 0, STATUS_STATUS = 1 };
enum { BRANCH_NAME = 0, BRANCH_HEAD, BRANCH_HASH, BRANCH_DATE, BRANCH_TIME, BRANCH_MAIL, BRANCH_DESC };
enum { LOG_HASH = 0, LOG_MAIL, LOG_DATE, LOG_TIME, LOG_DESC };
enum { DIFF_NAME = 0, DIFF_STATUS };

static bool setClipboard(std::wstring text) {
	if (::OpenClipboard(g_app.m_mainWindow) == 0) {
		return false;
	}
	::EmptyClipboard();

	size_t sizeInBytes = text.size() * sizeof(wchar_t);
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes + 1);
	if (hglbCopy != NULL) {
		char * buffer = (char *) GlobalLock(hglbCopy);
		if (buffer == nullptr) {
			return false;
		}
		memcpy(buffer, text.c_str(), sizeInBytes);
		buffer[sizeInBytes] = 0;
		GlobalUnlock(hglbCopy);

		::SetClipboardData(CF_UNICODETEXT, hglbCopy);
	}
	::CloseClipboard();
	return true;

}

} // namespace


LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;
		case WM_KILLFOCUS:
#ifndef _DEBUG
			DestroyWindow(hwnd);
#endif
			return 0;
		case WM_CREATE:
			g_app.OnCreate(hwnd);
			return 0;
		case WM_SIZE:
			g_app.OnSize(hwnd);
			return 0;
		case WM_CHAR:
			g_app.OnChar(wParam);
			return 0;
		case WM_KEYDOWN:
			g_app.OnKeyDown(hwnd, wParam);
			return 0;
		case WM_SYSKEYDOWN:
			g_app.OnSysKeyDown(hwnd, wParam);
			return 0;
			//case WM_ERASEBKGND:
		//	return 1;
		case WM_PAINT:
			g_app.OnPaint(hwnd);
			return 0;
		//case WM_MOUSEMOVE:
		//	g_app.OnMouseMove(hwnd, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		//	return 0;
		//case WM_LBUTTONDOWN:
		//	g_app.OnLButtonDown(hwnd, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		//	return 0;
		//case WM_LBUTTONUP:
		//	g_app.OnLButtonUp(hwnd, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		//	return 0;
		//case WM_RBUTTONUP:
		//	g_app.OnRButtonUp(hwnd, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		//	return 0;
		//case WM_MOUSEWHEEL:
		//	g_app.OnMouseWheel(hwnd, GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
		//	return 0;
		//case WM_COMMAND:
		//	if (HIWORD(wParam) == 0) {
		//		g_app.OnMenuCommand(hwnd, LOWORD(wParam));
		//		return 0;
		//	}
		//	return DefWindowProc(hwnd, message, wParam, lParam);
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
 }

void Application::OnCreate(HWND hwnd) {
	m_mainWindow = hwnd;

	fontFamily = std::make_unique<Gdiplus::FontFamily>(L"Lucida Console");
	font = std::make_unique<Gdiplus::Font>(fontFamily.get(), 11.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

	nmfontFamily = std::make_unique<Gdiplus::FontFamily>(L"Segoe UI");
	nmfont = std::make_unique<Gdiplus::Font>(nmfontFamily.get(), 11.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
}

void Application::OnPaint(HWND hwnd) {
	RECT rect;
	GetClientRect(hwnd, &rect);

	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;
	if (bitmap == NULL || bitmap->GetWidth() != width || bitmap->GetHeight() != height) {
		delete bitmap;
		bitmap = new Gdiplus::Bitmap(width, height);
	}

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	Gdiplus::Graphics graphics(hdc);
	Gdiplus::Graphics * g1 = graphics.FromImage(bitmap);
	g1->Clear(Gdiplus::Color(255, 230, 230, 230));
	paint(*g1);
	graphics.DrawImage(bitmap, 0, 0);
	EndPaint(hwnd, &ps);
}

void Application::OnSize(HWND hwnd) {
	RECT rect;
	GetClientRect(hwnd, &rect);
	layout(Gdiplus::RectF((Gdiplus::REAL) rect.left, (Gdiplus::REAL) rect.top, (Gdiplus::REAL) (rect.right - rect.left), (Gdiplus::REAL) (rect.bottom - rect.top)) );
	RedrawWindow(m_mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void Application::OnSysKeyDown(HWND hwnd, WPARAM wparam) {
	switch (wparam) {
		case VK_LEFT: {
			PrevView();
			return;
		}

		case VK_RIGHT: {
			NextView();
			return;
		}
	}
}

void Application::OnKeyDown(HWND hwnd, WPARAM wparam) {
	m_message.clear();
	const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

	switch (wparam) {
		case VK_BACK: {
			m_searchMode = false;
			if (m_filterString.length() > 0) {
				m_filterString.erase(m_filterString.end() - 1);
				UpdateFilter();
				InvalidateRect(hwnd, NULL, FALSE);
			}
			return;
		}

		case VK_ESCAPE: {
			if (m_filterString.length() > 0) {
				m_filterString.clear();
				UpdateFilter();
				InvalidateRect(hwnd, NULL, FALSE);
			} else {
				DestroyWindow(hwnd);
			}
			return;
		}

		case VK_TAB: {
			if (GetCurrentMode() == STATUS) {
				PushView({BRANCH, currentItem, L"", L""});
			} else {
				PushView({STATUS, currentItem, L"", L""});
			}
			return;
		}

		case 'R': {
			if (ctrl) {
				Refresh();
				return;
			}
			break;
		}
	}

	if (currentItem < 0 || currentItem >= m_items.size()) {
		if (m_items.empty()) {
			return;
		}
		currentItem = 0;
		ScrollIntoView();
		InvalidateRect(hwnd, NULL, FALSE);
	}

	switch (wparam) {

		//case VK_LEFT:
		//	if (filterCursor > 0) {
		//		--filterCursor;
		//		InvalidateRect(hwnd, NULL, FALSE);
		//	}
		//	break;

		//case VK_RIGHT:
		//	if (filterCursor < m_filterString.length()) {
		//		++filterCursor;
		//		InvalidateRect(hwnd, NULL, FALSE);
		//	}
		//	break;

		//case VK_DELETE:
		//	if (m_filterString.length() > filterCursor) {
		//		m_filterString = m_filterString.erase(filterCursor, 1);
		//		InvalidateRect(hwnd, NULL, FALSE);
		//	}
		//	break;

		case VK_HOME: {
			m_searchMode = false;
			currentItem = 0;
			if (!m_items[currentItem].match) {
				SelectNextMatch();
			}
			ScrollIntoView();
			InvalidateRect(hwnd, NULL, FALSE);
			return;
		}

		case VK_END: {
			m_searchMode = false;
			currentItem = (int) m_items.size() - 1;
			if (!m_items[currentItem].match) {
				SelectPrevMatch();
			}
			ScrollIntoView();
			InvalidateRect(hwnd, NULL, FALSE);
			return;
		}

		case VK_UP: {
			m_searchMode = false;
			if (SelectPrevMatch()) {
				ScrollIntoView();
				InvalidateRect(hwnd, NULL, FALSE);
			}
			return;
		}

		case VK_PRIOR: {
			m_searchMode = false;
			const int oldItem = currentItem;
			currentItem = currentItem <= m_pagesize ? 0 : currentItem - m_pagesize;
			if (!m_items[currentItem].match) {
				if (!SelectNextMatch() || currentItem > oldItem) {
					currentItem = oldItem;
					SelectPrevMatch();
				}
			}
			ScrollIntoView();
			InvalidateRect(hwnd, NULL, FALSE);
			return;
		}

		case VK_NEXT: {
			m_searchMode = false;
			const int oldItem = currentItem;
			currentItem = currentItem + m_pagesize < (int) m_items.size() ? currentItem + m_pagesize : (int) m_items.size() - 1;
			if (!m_items[currentItem].match) {
				if (!SelectPrevMatch() || currentItem < oldItem) {
					currentItem = oldItem;
					SelectNextMatch();
				}
			}
			ScrollIntoView();
			InvalidateRect(hwnd, NULL, FALSE);
			return;
		}

		case VK_DOWN: {
			if (SelectNextMatch()) {
				ScrollIntoView();
				InvalidateRect(hwnd, NULL, FALSE);
			}
			return;
		}
	}

	// Status

	if (GetCurrentMode() == STATUS) {
		switch (wparam) {
			case VK_SPACE: {
				if (!m_searchMode) {
					m_items[currentItem].selected = !m_items[currentItem].selected;
					InvalidateRect(hwnd, NULL, FALSE);
					return;
				}
				break;
			}

			case VK_MULTIPLY: {
				for (int i = 0; i < m_items.size(); ++i) {
					m_items[i].selected = !m_items[i].selected;
				}
				InvalidateRect(hwnd, NULL, FALSE);
				return;
			}

			case VK_ADD: {
				if (!m_searchMode) {
					if (ctrl) {
						for (int i = 0; i < m_items.size(); ++i) {
							m_items[i].selected = true;
						}
					} else {
						for (int i = 0; i < m_items.size(); ++i) {
							if (m_items[i].text[STATUS_STATUS][0] != L'?') {
								m_items[i].selected = true;
							}
						}
					}
					InvalidateRect(hwnd, NULL, FALSE);
					return;
				}
				break;
			}

			case VK_SUBTRACT: {
				if (!m_searchMode) {
					if (ctrl) {
						for (int i = 0; i < m_items.size(); ++i) {
							m_items[i].selected = false;
						}
					} else {
						for (int i = 0; i < m_items.size(); ++i) {
							if (m_items[i].text[STATUS_STATUS][0] != L'?') {
								m_items[i].selected = false;
							}
						}
					}
					InvalidateRect(hwnd, NULL, FALSE);
					return;
				}
				break;
			}

			case VK_INSERT: {
				m_items[currentItem].selected = !m_items[currentItem].selected;
				if (currentItem + 1 < (int) m_items.size()) {
					++currentItem;
					ScrollIntoView();
				}
				InvalidateRect(hwnd, NULL, FALSE);
				return;
			}

			case VK_RETURN: {
				std::wstring str;
				for (int i = 0; i < m_items.size(); ++i) {
					if (!m_items[i].selected) {
						continue;
					}
					if (str.empty()) {
						str += m_items[i].text[STATUS_NAME];
					} else {
						str += L" ";
						str += m_items[i].text[STATUS_NAME];
					}
				}
				if (str.empty()) {
					SendToTerminal(m_items[currentItem].text[STATUS_NAME]);
				}
				SendToTerminal(str);
				DestroyWindow(hwnd);
				return;
			}

			case VK_F3: {
				bool diffStaged = false;
				if (m_items[currentItem].text[STATUS_STATUS][0] == 'M' && m_items[currentItem].text[STATUS_STATUS][1] == ' ') {
					diffStaged = true;
				}

				std::wstring filename = m_items[currentItem].text[STATUS_NAME];

				m_message = diffStaged ? L"git difftool --staged " + filename : L"git difftool " + filename;
				RedrawWindow(m_mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				if (diffStaged) {
					exec(m_root, m_git + L" difftool --staged " + filename, [](const char * data, DWORD size) {});
				} else {
					exec(m_root, m_git + L" difftool " + filename, [](const char * data, DWORD size) {});
				}
				m_message += L" [DONE]";
				Refresh();
				return;
			}

			case VK_F4: {
				exec(m_root, m_sublime + L" " + m_root + L"\\" + m_items[currentItem].text[STATUS_NAME], [](const char * data, DWORD size) {});
				Refresh();
				return;
			}

			case VK_F5: {
				exec(m_root, m_git + L" add " + m_items[currentItem].text[STATUS_NAME], [](const char * data, DWORD size) {});
				Refresh();
				return;
			}

			case VK_F7: {
				exec(m_root, m_git + L" restore --staged " + m_items[currentItem].text[STATUS_NAME], [](const char * data, DWORD size) {});
				Refresh();
				return;
			}

			case 'C': {
				if (ctrl) {
					std::wstring filename = m_items[currentItem].text[STATUS_NAME];
					setClipboard(m_root + L"\\" + filename);
					DestroyWindow(hwnd);
					return;
				}
				break;
			}

			case 'O': {
				if (ctrl) {
					std::wstring filename = m_items[currentItem].text[STATUS_NAME];
					VisualStudioInterop::OpenInVS(filename);
					DestroyWindow(hwnd);
					return;
				}
				break;
			}

			case 'X': {
				const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
				if (ctrl) {
					exec(m_root, m_git + L" rm " + m_items[currentItem].text[STATUS_NAME], [](const char * data, DWORD size) {});
					Refresh();
					return;
				}
				break;
			}
		}
	}

	if (GetCurrentMode() == BRANCH) {
		switch (wparam) {
			case VK_RETURN: {
				if (ctrl) {
					SendToTerminal(L"git switch " + m_items[currentItem].text[BRANCH_NAME]);
					DestroyWindow(hwnd);
				} else {
					PushView({LOG, currentItem, m_items[currentItem].text[BRANCH_NAME], L"", L"log " + m_items[currentItem].text[BRANCH_NAME] });
					//SendToTerminal(m_items[currentItem].text[BRANCH_NAME]);
					//DestroyWindow(hwnd);
				}
				return;
			}

			case 'C': {
				if (ctrl) {
					setClipboard(m_items[currentItem].text[BRANCH_NAME]);
					DestroyWindow(hwnd);
					return;
				}
				break;
			}

			case 'F': {
				if (ctrl) {
					const std::wstring & branch = m_items[currentItem].text[BRANCH_NAME];
					SendToTerminal(L"git fetch origin " + branch + L":" + branch);
					DestroyWindow(hwnd);
					return;
				}
				break;
			}

			case 'L': {
				if (ctrl) {
					PushView({LOG, currentItem, m_items[currentItem].text[BRANCH_NAME], L"", L"log " + m_items[currentItem].text[BRANCH_NAME] });
					return;
				}
				break;
			}

			case 'O': {
				if (ctrl) {
					SendToTerminal(L"git switch " + m_items[currentItem].text[BRANCH_NAME]);
					DestroyWindow(hwnd);
					return;
				}
				break;
			}

			case 'X': {
				if (ctrl) {
					SendToTerminal(L"git branch -D " + m_items[currentItem].text[BRANCH_NAME]);
					DestroyWindow(hwnd);
					return;
				}
				break;
			}
		}
	}

	if (GetCurrentMode() == LOG) {
		switch (wparam) {
			case VK_RETURN: {
				if (ctrl) {
					SendToTerminal(m_items[currentItem].text[LOG_HASH]);
					DestroyWindow(hwnd);
				} else {
					PushView({DIFFTREE, currentItem, m_items[currentItem].text[LOG_HASH], L"", L"diff-tree " + m_items[currentItem].text[LOG_HASH].substr(0, 7) + L" " + m_items[currentItem].text[LOG_DESC]});
				}
				return;
			}

			case 'O': {
				if (ctrl) {
					PushView({DIFFTREE, currentItem, m_items[currentItem].text[LOG_HASH], L"", L"diff-tree " + m_items[currentItem].text[LOG_HASH].substr(0, 7) + L" " + m_items[currentItem].text[LOG_DESC]});
					return;
				}
				break;
			}
		}
	}

	if (GetCurrentMode() == DIFFTREE) {
		switch (wparam) {
			case VK_RETURN: {
				if (ctrl) {
					SendToTerminal(m_items[currentItem].text[DIFF_NAME]);
					DestroyWindow(hwnd);
				} else {
					PushView({LOG, currentItem, GetCurrentView().rev, m_items[currentItem].text[DIFF_NAME], L"log " + m_items[currentItem].text[DIFF_NAME] });
				}
				return;
			}

			case VK_F3: {
				exec(m_root, m_git + L" difftool " + GetCurrentView().rev + L"^ " + GetCurrentView().rev + L" -- " + m_items[currentItem].text[DIFF_NAME], [](const char * data, DWORD size) {});
				return;
			}

			case 'L': {
				if (ctrl) {
					PushView({LOG, currentItem, GetCurrentView().rev, m_items[currentItem].text[DIFF_NAME], L"log " + m_items[currentItem].text[DIFF_NAME] });
					return;
				}
				break;
			}
		}
	}
}

void Application::OnChar(WPARAM wparam) {
	if (!m_searchMode) {
		if (wparam == '+' || wparam == '-' || wparam == ' ' || wparam == '*') {
			return;
		}
	}
	if (isgraph((int) wparam) || wparam == ' ') {
		m_searchMode = true;
		m_filterString.push_back((wchar_t) wparam);
		UpdateFilter();
		InvalidateRect(m_mainWindow, NULL, FALSE);
		return;
	}
}

void Application::layout(const Gdiplus::RectF & rect) {
	float x = rect.GetLeft();
	float y = rect.GetTop();

	statusbarRect = rect;
	statusbarRect.Y = rect.GetBottom() - 16;
	statusbarRect.Height = 16;

	scrollbarRect = rect;
	scrollbarRect.X = rect.GetRight() - 10;
	scrollbarRect.Width = 10;
	scrollbarRect.Height = rect.Height - 18;

	itemsRect = rect;
	itemsRect.Width -= 10;
	itemsRect.Height -= 18;

	if (m_items.size() > 0) {
		m_pagesize = (int) (itemsRect.Height / ITEM_HEIGHT) - 1;
		if (m_pagesize <= 0) {
			m_pagesize = DEFAULT_PAGE_SIZE;
		}
	}
}

std::wstring format(const std::wstring & text, size_t len) {
	const size_t textLen = text.size();
	if (textLen > len) {
		return text.substr(0, len);
	}
	if (textLen == len) {
		return text;
	}
	return text + std::wstring(len - textLen, L' ');
}

void Application::paint(Gdiplus::Graphics & graphics) {

	Gdiplus::RectF itemRect = itemsRect;
	itemRect.Y = 0;
	itemRect.Height = ITEM_HEIGHT;

	for (int i = scrollOffset; i < m_items.size(); ++i) {
		if (i > scrollOffset + m_pagesize) {
			break;
		}
		m_items[i].paint(graphics, itemRect, m_formatter, currentItem == i);
		itemRect.Y += ITEM_HEIGHT;
	}

	Gdiplus::SolidBrush fg(Gdiplus::Color(255, 0, 0, 0));
	//Gdiplus::SolidBrush bgScrollbar(Gdiplus::Color(255, 240, 240, 240));
	//Gdiplus::SolidBrush fgScrollbar(Gdiplus::Color(255, 205, 205, 205));
	//Gdiplus::SolidBrush hoverScrollbar(Gdiplus::Color(255, 166, 166, 166));
	//Gdiplus::SolidBrush pressedScrollbar(Gdiplus::Color(255, 96, 96, 96));

	Gdiplus::SolidBrush bgScrollbar(Gdiplus::Color(255, 205, 205, 205));
	Gdiplus::SolidBrush fgScrollbar(Gdiplus::Color(255, 166, 166, 166));

	Gdiplus::SolidBrush bgStatus(Gdiplus::Color(255, 180, 190, 210));
	Gdiplus::SolidBrush bgFilter(Gdiplus::Color(255, 255, 255, 255));

	graphics.FillRectangle(&bgScrollbar, scrollbarRect);

	float start = scrollOffset / (float) m_items.size();
	float end = (scrollOffset + m_pagesize) / (float) m_items.size();

	Gdiplus::RectF scrollhandleRect = scrollbarRect;
	scrollhandleRect.Y = start * scrollbarRect.Height;
	scrollhandleRect.Height = (end - start) * scrollbarRect.Height;
	graphics.FillRectangle(&fgScrollbar, scrollhandleRect);

	Gdiplus::PointF point;
	statusbarRect.GetLocation(&point);
	point.Y += 2.0f;

	// these are probably three different things: a status bar, a function bar, and a search field...
	if (!m_filterString.empty()) {
		graphics.FillRectangle(&bgFilter, statusbarRect);
		graphics.DrawString(m_filterString.c_str(), -1, nmfont.get(), point, &fg);

		Gdiplus::RectF box;
		Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericDefault());
		format.SetFormatFlags(format.GetFormatFlags() | Gdiplus::StringFormatFlags::StringFormatFlagsMeasureTrailingSpaces);
		graphics.MeasureString(m_filterString.c_str(), -1, nmfont.get(), point, &format, &box);

		// draw custom caret.
		Gdiplus::Pen pen(&fg, 1.0f);
		Gdiplus::PointF cursor1(box.X + box.Width - 2.0f, box.Y);
		Gdiplus::PointF cursor2 = cursor1;
		cursor2.Y += box.Height;
		graphics.DrawLine(&pen, cursor1, cursor2);
	} else {
		if (!m_message.empty()) {
			graphics.DrawString(m_message.c_str(), -1, nmfont.get(), point, &fg);
		} else {
			graphics.FillRectangle(&bgStatus, statusbarRect);
			if (GetCurrentMode() == STATUS) {
				bool diffStaged = false;
				if (0 <= currentItem && currentItem < m_items.size()) {
					if (m_items[currentItem].text[STATUS_STATUS][0] == 'M' && m_items[currentItem].text[STATUS_STATUS][1] == ' ') {
						diffStaged = true;
					}
				}
				if (diffStaged) {
					graphics.DrawString(L"   F3 Diff [s]   F4 Sublime   F5 Add   F7 Restore   Ctrl+C Copy   Ctrl+R Refresh   Ctrl+O Open   Ctrl+X Delete", -1, nmfont.get(), point, &fg);
				} else {
					graphics.DrawString(L"   F3 Diff [H]   F4 Sublime   F5 Add   F7 Restore   Ctrl+C Copy   Ctrl+R Refresh   Ctrl+O Open   Ctrl+X Delete", -1, nmfont.get(), point, &fg);
				}
			} else if (GetCurrentMode() == BRANCH) {
				graphics.DrawString(L"   Ctrl+C Copy   Ctrl+F Fetch   Ctrl+L Log   Ctrl+O Switch   Ctrl+X Delete", -1, nmfont.get(), point, &fg);
			} else if (GetCurrentMode() == LOG) {
				graphics.DrawString(L"   Ctrl+C Copy   Ctrl+F Fetch   Ctrl+L Log   Ctrl+O Switch   Ctrl+X Delete", -1, nmfont.get(), point, &fg);
			} else if (GetCurrentMode() == DIFFTREE) {
			}
		}
	}
}

void Application::SendToTerminal(const std::wstring & str) {
	std::wcout << str;
}

void Application::UpdateFilter() {
	if (m_filterString.empty()) {
		for (int i = 0; i < m_items.size(); ++i) {
			m_items[i].match = true;
		}
		return;
	}

	std::wstring filterString = m_filterString;
	std::transform(filterString.begin(), filterString.end(), filterString.begin(), ::toupper);

	std::wistringstream iss(filterString);
	std::vector<std::wstring> split((std::istream_iterator<std::wstring, wchar_t>(iss)),
									 std::istream_iterator<std::wstring, wchar_t>());

	int best = 0;
	int score = 0;
 	for (int i = 0; i < m_items.size(); ++i) {
		std::wstring text = m_items[i].text[0];
 		std::transform(text.begin(), text.end(), text.begin(), ::toupper);

		m_items[i].match = true;

		int bestcand = best;
		int bestscore = score;
		for (int j = 0; j < split.size(); ++j) {
			size_t pos = text.find(split[j]);
			if (pos == std::wstring::npos) {
				m_items[i].match = false;
				break;
			}
			if (pos == 0 && bestscore < 3) {
				bestcand = i;
				bestscore = 3;
			} else if (pos > 0 && text[pos - 1] == '/' && score < 2) {
				bestcand = i;
				bestscore = 2;
			} else if (bestscore < 1) {
				bestcand = i;
				bestscore = 1;
			}
		}
		if (m_items[i].match) {
			best = bestcand;
			score = bestscore;
		}
	}

	currentItem = best;
	//if (0 <= currentItem && currentItem < m_items.size()) {
	//	if (!m_items[currentItem].match) {
	//		if (SelectNextMatch()) {
	//			return;
	//		}
	//		SelectPrevMatch();
	//	}
	//}
}

bool Application::SelectNextMatch() {
	for (int i = currentItem + 1; i < m_items.size(); ++i) {
		if (m_items[i].match) {
			currentItem = i;
			return true;
		}
	}
	return false;
}

bool Application::SelectPrevMatch() {
	for (int i = currentItem - 1; i >= 0; --i) {
		if (m_items[i].match) {
			currentItem = i;
			return true;
		}
	}
	return false;
}

void Application::ScrollIntoView() {
	if (currentItem < 0 || currentItem >= m_items.size()) {
		return;
	}
	if (currentItem < scrollOffset) {
		scrollOffset = currentItem;
	} else if (currentItem > scrollOffset + m_pagesize) {
		scrollOffset = currentItem - m_pagesize;
	}
}

void Application::Refresh() {
	m_items.clear();

	switch (GetCurrentMode()) {
		case STATUS:   GitStatus();   break;
		case BRANCH:   GitBranch();   break;
		case LOG:      GitLog();      break;
		case DIFFTREE: GitDiffTree(); break;
	}

	if (currentItem >= m_items.size()) {
		currentItem = (int) (m_items.size() - 1);
	}
	UpdateFilter();
	InvalidateRect(m_mainWindow, NULL, FALSE);
}

void Application::GitStatus() {
	m_formatter = [](const std::vector<std::wstring> & item) -> std::wstring {
		return item[1] + item[0];
	};

	const std::wstring cmd = m_git + L" status --branch --porcelain";
	const std::vector<std::wstring> files = exec(m_root, cmd);
	SetWindowText(m_mainWindow, (std::wstring(L"git status (") + files[0].c_str() + L")").c_str());
	for (size_t i = 1; i < files.size(); ++i) {
		std::wstring status = files[i].substr(0, 3);
		std::wstring name = files[i].substr(3);
		m_items.emplace_back(Item({name, status}));
	}
}

void Application::GitBranch() {
	m_formatter = [](const std::vector<std::wstring> & item) -> std::wstring {
		return item[1] + format(item[0], 25) + L" " + item[2].substr(0, 7) + L" " + item[3] + L" " + item[4] + L" " + item[5] + L" " + item[6];
	};

	const std::wstring cmd = m_git + L" for-each-ref --format=\"%(HEAD)%(refname:lstrip=-1) %(objectname) %(authordate:iso) %(authoremail) %(subject)\" refs/heads";
	const std::vector<std::wstring> files = exec(m_root, cmd);
	SetWindowText(m_mainWindow, L"Branch");
	for (size_t i = 0; i < files.size(); ++i) {
		// "*name hash 2020-07-28 23:24:14 +0200 <name@mail.com> description"
		const size_t nameIdx = 1;
		const size_t hashIdx = files[i].find(L' ', nameIdx) + 1;
		const size_t dateIdx = files[i].find(L' ', hashIdx) + 1;
		const size_t timeIdx = files[i].find(L' ', dateIdx) + 1;
		const size_t zoneIdx = files[i].find(L' ', timeIdx) + 1;
		const size_t mailIdx = files[i].find(L' ', zoneIdx) + 1;
		const size_t descIdx = files[i].find(L' ', mailIdx) + 1;
		std::wstring head{files[i][0]};
		std::wstring name = files[i].substr(nameIdx, hashIdx - nameIdx - 1);
		std::wstring hash = files[i].substr(hashIdx, dateIdx - hashIdx - 1);
		std::wstring date = files[i].substr(dateIdx, timeIdx - dateIdx - 1);
		std::wstring time = files[i].substr(timeIdx, zoneIdx - timeIdx - 1);
		std::wstring mail = files[i].substr(mailIdx + 1, descIdx - mailIdx - 3);
		std::wstring desc = files[i].substr(descIdx);
		m_items.emplace_back(Item({name, head, hash, date, time, mail, desc}));
	}
}

void Application::GitLog() {
	m_formatter = [](const std::vector<std::wstring> & item) -> std::wstring {
		return item[0].substr(0, 7) + L" " + format(item[1], 30) + L" " + item[2] + L" " + item[3] + L" " + item[4];
	};

	const std::wstring cmd = m_git + L" log  --max-count=20 --first-parent --pretty=format:\"%H %ai %aE %s\" " + GetCurrentView().rev + (GetCurrentView().path.empty() ? L"" : (L" -- " + GetCurrentView().path));
	const std::vector<std::wstring> files = exec(m_root, cmd);
	//SetWindowText(m_mainWindow, (std::wstring(L"log ") + GetCurrentView().rev + (GetCurrentView().path.empty() ? L"" : (L" -- " + GetCurrentView().path))).c_str());
	SetWindowText(m_mainWindow, GetCurrentView().desc.c_str());
	for (size_t i = 0; i < files.size(); ++i) {
		const size_t hashIdx = 0;
		const size_t dateIdx = files[i].find(L' ', hashIdx) + 1;
		const size_t timeIdx = files[i].find(L' ', dateIdx) + 1;
		const size_t zoneIdx = files[i].find(L' ', timeIdx) + 1;
		const size_t mailIdx = files[i].find(L' ', zoneIdx) + 1;
		const size_t descIdx = files[i].find(L' ', mailIdx) + 1;
		std::wstring hash = files[i].substr(hashIdx, dateIdx - hashIdx - 1);
		std::wstring date = files[i].substr(dateIdx, timeIdx - dateIdx - 1);
		std::wstring time = files[i].substr(timeIdx, zoneIdx - timeIdx - 1);
		std::wstring mail = files[i].substr(mailIdx, descIdx - mailIdx - 1);
		std::wstring desc = files[i].substr(descIdx);
		m_items.emplace_back(Item({hash, mail, date, time, desc}));
	}
}

void Application::GitDiffTree() {
	m_formatter = [](const std::vector<std::wstring> & item) -> std::wstring {
		return item[1] + L" " + item[0];
	};

	const std::wstring cmd = m_git + L" diff-tree -M --no-commit-id --name-status -r " + GetCurrentView().rev + (GetCurrentView().path.empty() ? L"" : GetCurrentView().path);
	const std::vector<std::wstring> files = exec(m_root, cmd);
	//SetWindowText(m_mainWindow, (std::wstring(L"difftree ") + GetCurrentView().rev + (GetCurrentView().path.empty() ? L"" : GetCurrentView().path)).c_str());
	SetWindowText(m_mainWindow, GetCurrentView().desc.c_str());
	for (size_t i = 0; i < files.size(); ++i) {
		std::wstring status = files[i].substr(0, 1);
		std::wstring name = files[i].substr(2);
		m_items.emplace_back(Item({name, status}));
	}
}
