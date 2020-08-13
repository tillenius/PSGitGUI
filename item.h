#pragma once

#include <string>
#include <atomic>
#include <vector>
#include <functional>
#include <Windows.h>
#include <gdiplus.h>

class Formatter;

struct Item {

	Item(std::vector<std::wstring> && text) : text{text} {}
	Item(const std::wstring & text) : text{text} {}

	void paint(Gdiplus::Graphics & graphics, const Gdiplus::RectF & itemRect, std::function<std::wstring(std::vector<std::wstring>)> formatter, bool current);

	std::vector<std::wstring> text;

	bool hover = false;
	bool pressed = false;
	bool selected = false;

	bool match = true;
};
