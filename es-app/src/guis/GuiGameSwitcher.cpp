#include "guis/GuiGameSwitcher.h"

#include "components/ImageComponent.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "views/ViewController.h"
#include "CanicaActivity.h"
#include "FileData.h"
#include "InputConfig.h"
#include "LocaleES.h"
#include "SaveStateRepository.h"
#include "SystemData.h"
#include "Window.h"
#include <algorithm>
#include <sys/stat.h>

#define RECENT 15
#define REQUEST_AGE 300		// a request older than this is from another time: it opens nothing

using TerminalStyle::alpha;

const std::string GuiGameSwitcher::REQUEST = "/var/run/canica/switcher-request";
const std::string GuiGameSwitcher::AUTOSAVES = "/var/run/canica/game-autosaves";

static std::string when(time_t moment)
{
	if (moment <= 0)
		return "";

	time_t now = time(NULL);
	struct tm local, today;
	localtime_r(&moment, &local);
	localtime_r(&now, &today);

	char clock[8];
	strftime(clock, sizeof(clock), "%H:%M", &local);

	long days = (long)((now - moment) / 86400);
	if (local.tm_yday == today.tm_yday && local.tm_year == today.tm_year)
		return std::string("hoy ") + clock;
	if (days <= 1)
		return std::string("ayer ") + clock;
	if (days < 30)
		return "hace " + std::to_string(days) + " días";

	char date[16];
	strftime(date, sizeof(date), "%d/%m/%Y", &local);
	return date;
}

std::vector<GuiGameSwitcher::Entry> GuiGameSwitcher::recentGames(size_t count)
{
	std::vector<std::pair<std::string, FileData*>> played;

	for (auto system : SystemData::sSystemVector)
	{
		if (system->isCollection() || system->isGroupSystem() || SystemData::isUtilitySystem(system) || system->getRootFolder() == nullptr)
			continue;

		for (auto game : system->getRootFolder()->getFilesRecursive(GAME))
		{
			// Only a real date (YYYYMMDDTHHMMSS): a game never played has the default "0"
			std::string last = game->getMetadata(MetaDataId::LastPlayed);
			if (last.size() >= 15 && last[8] == 'T')
				played.push_back({ last, game });
		}
	}

	std::sort(played.begin(), played.end(), [](const std::pair<std::string, FileData*>& a, const std::pair<std::string, FileData*>& b)
	{
		return a.first > b.first;
	});

	std::vector<Entry> entries;
	for (auto& item : played)
	{
		if (entries.size() >= count)
			break;

		FileData* game = item.second;
		Entry entry = { game, "", false, 0 };

		if (SaveStateRepository::isEnabledForGame(game) && game->getCurrentGameSetting("autosave") == "1")
		{
			SaveState* state = game->getSourceFileData()->getSystem()->getSaveStateRepository()->getGameAutoSave(game);
			if (state != nullptr && state->slot == -1 && !state->fileName.empty())
			{
				entry.resumes = true;
				entry.image = state->screenshot;
				entry.saved = state->creationDate.getTime();
			}
		}

		if (entry.image.empty() || !Utils::FileSystem::exists(entry.image))
			entry.image = game->getImagePath();

		entries.push_back(entry);
	}

	return entries;
}

void GuiGameSwitcher::open(Window* window)
{
	auto entries = recentGames(RECENT);
	if (entries.empty())
	{
		window->displayNotificationMessage(_("NO RECENT GAMES"));
		return;
	}

	window->pushGui(new GuiGameSwitcher(window, entries));
}

void GuiGameSwitcher::openIfRequested(Window* window)
{
	struct stat info;
	if (stat(REQUEST.c_str(), &info) != 0)
		return;

	Utils::FileSystem::removeFile(REQUEST);
	if (time(NULL) - info.st_mtime <= REQUEST_AGE)
		open(window);
}

void GuiGameSwitcher::onGameStarting(FileData* game)
{
	// Only with the kit's run folder: its guard is the one that reads this
	if (!Utils::FileSystem::exists("/var/run/canica"))
		return;

	bool autosaves = game->getEmulator() == "libretro" && SaveStateRepository::isEnabledForGame(game) &&
		game->getCurrentGameSetting("autosave") == "1";

	if (autosaves)
		Utils::FileSystem::writeAllText(AUTOSAVES, game->getPath() + "\n");
	else
		Utils::FileSystem::removeFile(AUTOSAVES);
}

void GuiGameSwitcher::onGameEnded()
{
	if (Utils::FileSystem::exists(AUTOSAVES))
		Utils::FileSystem::removeFile(AUTOSAVES);
}

GuiGameSwitcher::GuiGameSwitcher(Window* window, const std::vector<Entry>& entries) : GuiComponent(window),
	mEntries(entries), mIndex(0), mBar(0), mFoot(0)
{
	float sw = (float)Renderer::getScreenWidth();
	float sh = (float)Renderer::getScreenHeight();
	setSize(sw, sh);

	mStyle = TerminalStyle::get(window);
	select(0);
}

GuiGameSwitcher::~GuiGameSwitcher()
{
}

void GuiGameSwitcher::select(int index)
{
	int count = (int)mEntries.size();
	mIndex = ((index % count) + count) % count;
	const Entry& entry = mEntries[mIndex];
	FileData* game = entry.game;

	float sw = (float)Renderer::getScreenWidth();
	float sh = (float)Renderer::getScreenHeight();
	auto font = mStyle.font;
	auto small = Font::get(Math::max(8, (int)Math::round(font->getSize() * 0.74f)), font->getPath());

	float margin = Math::round(Math::max(8.0f, (sw - mStyle.size.x()) / 2));
	float x = margin, y = Math::round(margin * 0.5f), w = sw - margin * 2, h = sh - y * 2 - Math::round(sh * 0.05f);
	mBox = Vector4f(x, y, w, h);

	float ch = font->sizeText("0").x();
	float line = font->getHeight(1.0f);
	float row = Math::round(line * 1.42f);
	float smallLine = small->getHeight(1.0f);
	float pad = Math::round(ch * 2);
	mBar = Math::round(smallLine * 2.3f);
	mFoot = Math::round(smallLine * 2.6f);

	unsigned int ink = mStyle.color;
	unsigned int muted = alpha(ink, 150);
	unsigned int accent = mStyle.fillColor;

	mText.clear();
	mText.add(small, TerminalStyle::spaced("CANICA // CONTINUAR"), x + pad, y + (mBar - smallLine) / 2, muted);
	mText.addRight(small, std::to_string(mIndex + 1) + " / " + std::to_string(count), x + w - pad, y + (mBar - smallLine) / 2, accent);

	// The picture: the automatic state's screenshot, else the game's image. Pixel art grows by whole steps.
	float infoHeight = row * 3;
	float pagerHeight = Math::round(line * 0.5f);
	mArea = Vector4f(x + pad + ch * 2, y + mBar + Math::round(row * 0.4f), w - (pad + ch * 2) * 2,
		h - mBar - mFoot - infoHeight - pagerHeight - Math::round(row * 1.2f));

	mImage = std::unique_ptr<ImageComponent>(new ImageComponent(mWindow, true));
	mImage->setImage(entry.image);

	Vector2i texture = mImage->getTextureSize();
	int scale = texture.x() > 0 && texture.y() > 0 ? (int)Math::min(mArea.z() / texture.x(), mArea.w() / texture.y()) : 0;
	if (scale >= 2)
	{
		mImage->setIsLinear(false);
		mImage->setResize((float)(texture.x() * scale), (float)(texture.y() * scale));
	}
	else
		mImage->setMaxSize(mArea.z(), mArea.w());

	Vector2f size = mImage->getSize();
	mImage->setPosition(Math::round(mArea.x() + (mArea.z() - size.x()) / 2), Math::round(mArea.y() + (mArea.w() - size.y()) / 2));

	// Left and right of the picture, in the margin
	float middle = mArea.y() + mArea.w() / 2 - line / 2;
	if (count > 1)
	{
		mText.add(font, "<", x + pad, middle, accent);
		mText.addRight(font, ">", x + w - pad, middle, accent);
	}

	// Under it: the game, where and how long it was played, and how it continues
	float ty = mArea.y() + mArea.w() + Math::round(row * 0.4f);
	float textWidth = w - pad * 2;
	mText.add(font, TerminalStyle::fit(font, game->getName(), textWidth), x + pad, ty, ink);
	ty += row;

	std::string details = game->getSourceFileData()->getSystem() ? game->getSourceFileData()->getSystem()->getFullName() : "";
	long seconds = atol(game->getMetadata(MetaDataId::GameTime).c_str());
	if (seconds > 0)
		details += " · jugado " + CanicaActivity::duration(seconds);

	std::string last = game->getMetadata(MetaDataId::LastPlayed);
	if (last.size() >= 15)
		details += " · " + when(Utils::Time::stringToTime(last));

	mText.add(font, TerminalStyle::fit(font, details, textWidth), x + pad, ty, muted);
	ty += row;

	if (entry.resumes)
	{
		mText.add(font, ">", x + pad, ty, accent);
		mText.add(font, TerminalStyle::fit(font, "partida automática · " + when(entry.saved), textWidth - ch * 2), x + pad + ch * 2, ty, accent);
	}
	else
		mText.add(font, "sin partida automática: empieza desde su guardado", x + pad, ty, muted);

	// One cell per game along the footer's rule
	float cell = Math::round(ch * 1.2f);
	mPager = Vector4f(x + w - pad - cell * count, y + h - mFoot - pagerHeight - Math::round(row * 0.45f), cell, pagerHeight);

	mText.add(small, TerminalStyle::spaced("A CONTINUAR   B VOLVER"), x + pad, y + h - mFoot + (mFoot - smallLine) / 2, accent);
	updateHelpPrompts();
}

bool GuiGameSwitcher::input(InputConfig* config, Input input)
{
	if (input.value == 0)
		return true;

	if (config->isMappedLike("left", input) || config->isMappedTo("pageup", input))
		select(mIndex - 1);
	else if (config->isMappedLike("right", input) || config->isMappedTo("pagedown", input))
		select(mIndex + 1);
	else if (config->isMappedTo(BUTTON_OK, input))
	{
		FileData* game = mEntries[mIndex].game;
		delete this;
		ViewController::get()->launch(game);
	}
	else if (config->isMappedTo(BUTTON_BACK, input) || (config->hotkeyHeld() && config->isMappedTo("select", input)))
	{
		if (config->hotkeyHeld())
			config->consumeHotkeyConfirm();

		delete this;
	}

	return true;
}

void GuiGameSwitcher::render(const Transform4x4f& /*parentTrans*/)
{
	TerminalStyle::drawFrame(mStyle, mBox.x(), mBox.y(), mBox.z(), mBox.w(), mBar, mFoot);

	// The picture sits in a dark well with a thin lit edge
	Renderer::drawSolidRectangle(mArea.x() - 4, mArea.y() - 4, mArea.z() + 8, mArea.w() + 8, 0x000000B0, alpha(mStyle.fillColor, 70), 1.0f, 3.0f);

	if (mImage)
	{
		Transform4x4f identity = Transform4x4f::Identity();
		mImage->render(identity);
	}

	for (int i = 0; i < (int)mEntries.size(); i++)
		Renderer::drawRect(mPager.x() + i * mPager.z(), mPager.y(), Math::max(1.0f, mPager.z() - 3), mPager.w(),
			i == mIndex ? mStyle.fillColor : mStyle.trackColor);

	mText.render();
	TerminalStyle::drawScanLines();
}

std::vector<HelpPrompt> GuiGameSwitcher::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	if (mEntries.size() > 1)
		prompts.push_back(HelpPrompt("left/right", _("CHOOSE")));

	prompts.push_back(HelpPrompt(BUTTON_OK, _("LAUNCH")));
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	return prompts;
}
