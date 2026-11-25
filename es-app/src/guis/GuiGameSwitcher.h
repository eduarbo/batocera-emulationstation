#pragma once
#ifndef ES_APP_GUIS_GUI_GAME_SWITCHER_H
#define ES_APP_GUIS_GUI_GAME_SWITCHER_H

#include "GuiComponent.h"
#include "math/Vector4f.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class FileData;
class ImageComponent;
class ThemeData;

// The recent games, one at a time, each as its automatic save left it: left and right (or L1 and R1) to go
// through them, A to continue. FN + SELECT opens it in the menu, and during a game too: the Canica kit's guard
// closes a game that keeps an automatic save state, and the menu opens this when it is back.
//
// The theme draws it, with elements of its screen view that are not extras (switcher*): the decoration, the
// texts the switcher fills, the area of the picture (switcherShot) and the pager's dots (switcherDot and
// switcherDotOn: their colour, size and the centre of the row).
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
	void setText(const std::string& part, const std::string& text);

	std::shared_ptr<ThemeData> mTheme;
	std::vector<Entry> mEntries;
	int mIndex;

	std::vector<std::shared_ptr<GuiComponent>> mParts;			// in the theme's order of depth
	std::map<std::string, std::shared_ptr<GuiComponent>> mTexts;
	std::unique_ptr<ImageComponent> mImage;
	Vector4f mArea;
	float mCorners;
	Vector4f mDots;			// centre x, centre y, size, step
	unsigned int mDotColor;
	unsigned int mDotOnColor;
};

#endif // ES_APP_GUIS_GUI_GAME_SWITCHER_H
