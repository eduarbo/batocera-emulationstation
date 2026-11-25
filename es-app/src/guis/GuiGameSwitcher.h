#pragma once
#ifndef ES_APP_GUIS_GUI_GAME_SWITCHER_H
#define ES_APP_GUIS_GUI_GAME_SWITCHER_H

#include "GuiComponent.h"
#include "guis/TerminalStyle.h"
#include "math/Vector4f.h"
#include <memory>
#include <string>
#include <vector>

class FileData;
class ImageComponent;

// The recent games, one at a time, each as its automatic save left it: left and right to go through them,
// A to continue. FN + SELECT opens it in the menu, and during a game too: the Canica kit's guard closes a game
// that keeps an automatic save state, and the menu opens this when it is back.
class GuiGameSwitcher : public GuiComponent
{
public:
	// The guard leaves REQUEST when FN + SELECT closed the game; the menu leaves AUTOSAVES while the running
	// game writes its automatic state when it is closed, so the guard knows that closing it loses nothing
	static const std::string REQUEST;
	static const std::string AUTOSAVES;

	static void open(Window* window);
	static void openIfRequested(Window* window);
	static void onGameStarting(FileData* game);
	static void onGameEnded();

	~GuiGameSwitcher();

	bool input(InputConfig* config, Input input) override;
	void render(const Transform4x4f& parentTrans) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	struct Entry
	{
		FileData* game;
		std::string image;
		bool resumes;
		time_t saved;
	};

	GuiGameSwitcher(Window* window, const std::vector<Entry>& entries);
	static std::vector<Entry> recentGames(size_t count);

	void select(int index);

	Window::PopupStyle mStyle;
	std::vector<Entry> mEntries;
	int mIndex;

	std::unique_ptr<ImageComponent> mImage;
	TerminalStyle::TextLayer mText;
	Vector4f mBox;
	Vector4f mArea;
	Vector4f mPager;		// x, y, cell, height
	float mBar;
	float mFoot;
};

#endif // ES_APP_GUIS_GUI_GAME_SWITCHER_H
