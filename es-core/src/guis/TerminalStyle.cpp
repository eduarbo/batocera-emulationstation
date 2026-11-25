#include "guis/TerminalStyle.h"

#include "math/Transform4x4f.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "ThemeData.h"

unsigned int TerminalStyle::alpha(unsigned int color, unsigned int a)
{
	return (color & 0xFFFFFF00) | (((color & 0xFF) * a) / 255);
}

std::string TerminalStyle::spaced(const std::string& text)
{
	std::string out;

	for (size_t i = 0; i < text.size(); )
	{
		size_t next = Utils::String::nextCursor(text, i);
		out += text.substr(i, next - i);
		if (next < text.size())
			out += ' ';

		i = next;
	}

	return out;
}

std::string TerminalStyle::fit(const std::shared_ptr<Font>& font, const std::string& text, float width)
{
	if (font->sizeText(text).x() <= width)
		return text;

	std::string cut = text;
	while (!cut.empty() && font->sizeText(cut + "…").x() > width)
		cut = cut.substr(0, Utils::String::prevCursor(cut, cut.size()));

	return Utils::String::trim(cut) + "…";
}

Window::PopupStyle TerminalStyle::get(Window* window)
{
	Window::PopupStyle style = window->getConsoleStyle();
	if (style.enabled && style.font != nullptr)
		return style;

	auto menu = ThemeData::getMenuTheme();
	float w = (float)Renderer::getScreenWidth();
	float h = (float)Renderer::getScreenHeight();

	style.enabled = true;
	style.pos = Vector2f(w / 2, h / 2);
	style.origin = Vector2f(0.5f, 0.5f);
	style.size = Vector2f(Math::round(w * 0.894f), Math::round(h * 0.742f));
	style.font = menu->TextSmall.font ? menu->TextSmall.font : Font::get(FONT_SIZE_SMALL);
	style.color = menu->Text.color;
	style.backgroundColor = (menu->Background.color & 0xFFFFFF00) | 0xF6;
	style.fillColor = menu->Text.selectorColor | 0xFF;
	style.borderColor = alpha(style.fillColor, 150);
	style.trackColor = alpha(menu->Text.color, 60);
	style.trackBorderColor = 0x000000C0;
	style.cornerRadius = 4;
	return style;
}

void TerminalStyle::drawFrame(const Window::PopupStyle& style, float x, float y, float w, float h, float bar, float foot)
{
	unsigned int accent = style.fillColor;

	Renderer::setMatrix(Transform4x4f::Identity());
	Renderer::drawRect(0.0f, 0.0f, (float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight(), style.trackBorderColor);

	for (int i = 3; i > 0; i--)
		Renderer::drawSolidRectangle(x - i * 2, y - i * 2, w + i * 4, h + i * 4, 0, alpha(accent, 22 * (4 - i)), 2.0f, style.cornerRadius + i * 2);

	Renderer::drawSolidRectangle(x, y, w, h, style.backgroundColor, style.borderColor, 1.0f, style.cornerRadius);

	if (bar > 0)
		Renderer::drawRect(x + 1, y + bar, w - 2, 1.0f, alpha(accent, 60));

	if (foot > 0)
		Renderer::drawRect(x + 1, y + h - foot, w - 2, 1.0f, alpha(accent, 60));
}

void TerminalStyle::drawScanLines()
{
	float w = (float)Renderer::getScreenWidth();
	float h = (float)Renderer::getScreenHeight();

	Renderer::setMatrix(Transform4x4f::Identity());
	for (float line = 0; line < h; line += 3)
		Renderer::drawRect(0.0f, line, w, 1.0f, 0x00000022);
}

void TerminalStyle::TextLayer::clear()
{
	for (auto& text : mTexts)
		delete text.second;

	mTexts.clear();
}

void TerminalStyle::TextLayer::add(const std::shared_ptr<Font>& font, const std::string& text, float x, float y, unsigned int color)
{
	if (!text.empty())
		mTexts.push_back({ font, font->buildTextCache(text, Math::round(x), Math::round(y), color) });
}

void TerminalStyle::TextLayer::addRight(const std::shared_ptr<Font>& font, const std::string& text, float right, float y, unsigned int color)
{
	add(font, text, right - font->sizeText(text).x(), y, color);
}

void TerminalStyle::TextLayer::render()
{
	for (auto& text : mTexts)
		text.first->renderTextCache(text.second);
}
