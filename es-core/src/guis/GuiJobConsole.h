#pragma once
#ifndef ES_CORE_GUIS_GUI_JOB_CONSOLE_H
#define ES_CORE_GUIS_GUI_JOB_CONSOLE_H

#include "GuiComponent.h"
#include "math/Vector4f.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Font;
class TextCache;

// A terminal window over the dimmed screen while an outside job works, such as the kit updater: its steps as a
// log, a gauge and a footer; or a question with two answers. It takes every input, so nothing starts under it.
// Its look and place come from the theme: the "console" notification element of the screen view.
class GuiJobConsole : public GuiComponent
{
public:
	struct Screen
	{
		Screen() : percent(-1) {}

		std::string title;
		std::string tag;
		std::string footer;
		std::vector<std::string> lines;		// "$ command", "label\tvalue\tok" (done) or "label\tvalue" (under way)
		int percent;						// below 0: no gauge
		std::vector<std::string> answers;	// a question: two labels, the first one focused, then the fields of
											// the row shown while the job takes over after the second answer
	};

	// On the UI thread. show() opens the console or replaces what it says; onAnswer gets 0 or 1.
	static void show(Window* window, const Screen& screen, const std::function<void(int)>& onAnswer = nullptr);
	static void close();

	~GuiJobConsole();

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	GuiJobConsole(Window* window);

	struct Text
	{
		std::shared_ptr<Font> font;
		TextCache* cache;
	};

	void set(const Screen& screen, const std::function<void(int)>& onAnswer);
	void layout();
	void clear();
	void add(const std::shared_ptr<Font>& font, const std::string& text, float x, float y, unsigned int color);
	void answer(int index);

	static GuiJobConsole* sInstance;

	Screen mScreen;
	std::function<void(int)> mOnAnswer;
	int mFocus;
	int mQuiet;			// ms since the job last said something
	int mQuietLimit;
	int mBlink;

	std::shared_ptr<Font> mFont;
	std::shared_ptr<Font> mSmallFont;
	std::vector<Text> mTexts;
	Vector4f mBox;
	float mBar;
	float mFoot;
	Vector4f mCursor;		// the block after the step under way; no width: none
	Vector4f mGauge;		// x, y, cell pitch, cell height; no pitch: none
	Vector4f mButtons[2];
};

#endif // ES_CORE_GUIS_GUI_JOB_CONSOLE_H
