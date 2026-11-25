#pragma once
#ifndef ES_APP_GUIS_GUI_ACTIVITY_H
#define ES_APP_GUIS_GUI_ACTIVITY_H

#include "GuiComponent.h"
#include "guis/TerminalStyle.h"
#include "math/Vector4f.h"
#include <vector>

// What was played, drawn as a terminal: totals, the last seven days, the most played games and the
// favourite hour and day (CanicaActivity)
class GuiActivity : public GuiComponent
{
public:
	GuiActivity(Window* window);

	bool input(InputConfig* config, Input input) override;
	void render(const Transform4x4f& parentTrans) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	struct Bar
	{
		float x, y, cell, height;
		int full;
	};

	Window::PopupStyle mStyle;
	TerminalStyle::TextLayer mText;
	Vector4f mBox;
	float mBar;
	float mFoot;
	std::vector<Bar> mBars;
};

#endif // ES_APP_GUIS_GUI_ACTIVITY_H
