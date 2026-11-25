#pragma once
#ifndef ES_CORE_GUIS_TERMINAL_STYLE_H
#define ES_CORE_GUIS_TERMINAL_STYLE_H

#include "Window.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Font;
class TextCache;

// What the screens drawn as a terminal share (the job console, the game switcher, the activity page): the
// theme's console style or one made from the menu, the window's chrome and its text.
namespace TerminalStyle
{
	unsigned int alpha(unsigned int color, unsigned int a);

	// Letters set apart, as on the title bar of an old terminal
	std::string spaced(const std::string& text);

	// The text cut to the width, with an ellipsis
	std::string fit(const std::shared_ptr<Font>& font, const std::string& text, float width);

	// The theme's console element when it styles one, else the menu's colours and font
	Window::PopupStyle get(Window* window);

	// The dimmed screen, the glow of a phosphor screen, the box and the rules under its title bar and over its footer
	void drawFrame(const Window::PopupStyle& style, float x, float y, float w, float h, float bar, float foot);

	// Scan lines over everything, barely there
	void drawScanLines();

	class TextLayer
	{
	public:
		~TextLayer() { clear(); }

		void clear();
		void add(const std::shared_ptr<Font>& font, const std::string& text, float x, float y, unsigned int color);
		void addRight(const std::shared_ptr<Font>& font, const std::string& text, float right, float y, unsigned int color);
		void render();

	private:
		std::vector<std::pair<std::shared_ptr<Font>, TextCache*>> mTexts;
	};
}

#endif // ES_CORE_GUIS_TERMINAL_STYLE_H
