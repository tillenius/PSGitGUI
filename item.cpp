#include "item.h"
#include "application.h"
#include <Windows.h>
#include <gdiplus.h>


void Item::paint(Gdiplus::Graphics & graphics, const Gdiplus::RectF & itemRect, std::function<std::wstring(std::vector<std::wstring>)> formatter, bool current) {

	Gdiplus::Color bgColor;
	if (current && !selected) {
		bgColor = Gdiplus::Color(255, 230, 180, 180);
	} else if (current && selected) {
		bgColor = Gdiplus::Color(255, 230, 140, 140);
	} else if (!current && selected) {
		bgColor = Gdiplus::Color(255, 255, 255, 200);
	} else if (!current && !selected) {
		bgColor = Gdiplus::Color(255, 230, 230, 230);
	}

	Gdiplus::SolidBrush fg(Gdiplus::Color(255, 0, 0, 0));
	Gdiplus::SolidBrush fgnomatch(Gdiplus::Color(255, 128, 128, 128));
	Gdiplus::SolidBrush bg(bgColor);

	const int paddingX = 10;
	const int paddingY = 0;

	graphics.FillRectangle(&bg, itemRect);
	Gdiplus::RectF clip = itemRect;
	clip.Inflate(-paddingX, -paddingY);
	Gdiplus::PointF point;
	clip.GetLocation(&point);
	point.Y += 2;

	std::wstring disp = formatter(text);

	if (match) {
		graphics.DrawString(disp.c_str(), -1, g_app.font.get(), point, &fg);
	} else {
		graphics.DrawString(disp.c_str(), -1, g_app.font.get(), point, &fgnomatch);
	}
}
