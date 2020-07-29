#pragma once

#include <string>
#include <atomic>
#include <Windows.h>
#include <gdiplus.h>

struct Item {

	Item() {}
	Item(const std::wstring & text) : text(text) {}

	void paint(Gdiplus::Graphics & graphics, const Gdiplus::RectF & itemRect, bool current);

	std::wstring text;

	bool hover = false;
	bool pressed = false;
	bool selected = false;

	bool match = true;
};
