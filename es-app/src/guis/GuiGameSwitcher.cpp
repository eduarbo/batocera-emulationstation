#include "guis/GuiGameSwitcher.h"

#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "views/ViewController.h"
#include "BindingManager.h"
#include "ThemeData.h"
#include "CanicaActivity.h"
#include "CanicaText.h"
#include "FileData.h"
#include "InputConfig.h"
#include "LocaleES.h"
#include "SaveStateRepository.h"
#include "SystemData.h"
#include "Window.h"
#include <algorithm>
#include <cstdlib>
#include <sys/stat.h>

#define RECENT 15
#define REQUEST_AGE 300		// a request older than this is from another time: it opens nothing

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

	const bool es = Canica::spanish();
	long days = (long)((now - moment) / 86400);
	if (local.tm_yday == today.tm_yday && local.tm_year == today.tm_year)
		return std::string(es ? "hoy " : "today ") + clock;
	if (days <= 1)
		return std::string(es ? "ayer " : "yesterday ") + clock;
	if (days < 30)
		return es ? "hace " + std::to_string(days) + " días" : std::to_string(days) + " days ago";

	char date[16];
	strftime(date, sizeof(date), es ? "%d/%m/%Y" : "%m/%d/%Y", &local);
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
				entry.saved = state->creationDate.getTime();

				// The screenshot only when it was taken with this save: another one shows another moment
				struct stat picture;
				if (!state->screenshot.empty() && stat(state->screenshot.c_str(), &picture) == 0 &&
					std::abs((long)(picture.st_mtime - entry.saved)) <= 120)
					entry.image = state->screenshot;
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

#define VIEW "switcher"

static std::shared_ptr<ThemeData> currentTheme()
{
	for (auto system : SystemData::sSystemVector)
		if (system->getTheme() != nullptr)
			return system->getTheme();

	return nullptr;
}

GuiGameSwitcher::GuiGameSwitcher(Window* window, const std::vector<Entry>& entries) : GuiComponent(window),
	mEntries(entries), mIndex(0), mCorners(0), mDotColor(0x43536BFF), mDotOnColor(0xCFCBFFFF)
{
	float sw = (float)Renderer::getScreenWidth();
	float sh = (float)Renderer::getScreenHeight();
	setSize(sw, sh);

	mTheme = currentTheme();
	mArea = Vector4f(sw * 0.33f, sh * 0.15f, sw * 0.64f, sh * 0.78f);
	mDots = Vector4f(sw * 0.65f, sh * 0.96f, 5, 11);

	if (mTheme)
	{
		mExtras = ThemeData::makeExtras(mTheme, VIEW, window, true);
		std::stable_sort(mExtras.begin(), mExtras.end(), [](GuiComponent* a, GuiComponent* b) { return a->getZIndex() < b->getZIndex(); });

		if (mTheme->getElement(VIEW, "switcherState", "text") != nullptr)
		{
			mState = std::make_shared<TextComponent>(window);
			mState->applyTheme(mTheme, VIEW, "switcherState", ThemeFlags::ALL);
		}

		auto shot = mTheme->getElement(VIEW, "switcherShot", "image");
		if (shot && shot->has("pos") && shot->has("size"))
		{
			Vector2f pos = shot->get<Vector2f>("pos"), size = shot->get<Vector2f>("size");
			mArea = Vector4f(pos.x() * sw, pos.y() * sh, size.x() * sw, size.y() * sh);
			if (shot->has("roundCorners"))
				mCorners = shot->get<float>("roundCorners");
		}

		auto dot = mTheme->getElement(VIEW, "switcherDot", "text");
		if (dot && dot->has("pos") && dot->has("size"))
		{
			Vector2f pos = dot->get<Vector2f>("pos"), size = dot->get<Vector2f>("size");
			mDots = Vector4f(pos.x() * sw, pos.y() * sh, Math::round(size.y() * sh), Math::round(size.x() * sw));
			if (dot->has("color"))
				mDotColor = dot->get<unsigned int>("color");
		}

		auto on = mTheme->getElement(VIEW, "switcherDotOn", "text");
		if (on && on->has("color"))
			mDotOnColor = on->get<unsigned int>("color");
	}

	select(0);
}

GuiGameSwitcher::~GuiGameSwitcher()
{
	for (auto extra : mExtras)
		delete extra;
}

void GuiGameSwitcher::select(int index)
{
	int count = (int)mEntries.size();
	mIndex = ((index % count) + count) % count;
	const Entry& entry = mEntries[mIndex];
	FileData* game = entry.game;

	// The column is the game list's: its elements take this game as a game list's take the selected one
	for (auto extra : mExtras)
		BindingManager::updateBindings(extra, game);

	if (mState)
		std::static_pointer_cast<TextComponent>(mState)->setText(entry.resumes ?
			Canica::text("Partida automática · ", "Auto save · ") + when(entry.saved) :
			Canica::text("Sin partida automática: abre desde su guardado", "No auto save: load it from its own save"));

	// The picture: the automatic state's screenshot, else the game's image. Pixel art grows by whole steps,
	// sharp; anything else fits the area.
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
	{
		mImage->setIsLinear(texture.x() > 400);
		mImage->setMaxSize(mArea.z(), mArea.w());
	}

	if (mCorners > 0)
		mImage->setRoundCorners(mCorners);

	Vector2f size = mImage->getSize();
	mImage->setPosition(Math::round(mArea.x() + (mArea.z() - size.x()) / 2), Math::round(mArea.y() + (mArea.w() - size.y()) / 2));
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

void GuiGameSwitcher::update(int deltaTime)
{
	for (auto extra : mExtras)
		extra->update(deltaTime);

	GuiComponent::update(deltaTime);
}

void GuiGameSwitcher::render(const Transform4x4f& /*parentTrans*/)
{
	Transform4x4f identity = Transform4x4f::Identity();

	// Without the theme's view, the menu under it is dimmed
	if (mExtras.empty())
	{
		Renderer::setMatrix(identity);
		Renderer::drawRect(0.0f, 0.0f, (float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight(), 0x000000E0);
	}

	// The picture goes over the background and the panel, under everything drawn above it
	bool drawn = false;
	for (auto extra : mExtras)
	{
		if (!drawn && extra->getZIndex() >= 30)
		{
			if (mImage)
				mImage->render(identity);
			drawn = true;
		}

		if (extra->isVisible())
			extra->render(identity);
	}

	if (!drawn && mImage)
		mImage->render(identity);

	if (mState)
		mState->render(identity);

	int count = (int)mEntries.size();
	if (count > 1)
	{
		Renderer::setMatrix(identity);
		float size = mDots.z(), step = mDots.w();
		float x = Math::round(mDots.x() - (step * (count - 1) + size) / 2);
		for (int i = 0; i < count; i++)
			Renderer::drawSolidRectangle(x + i * step, Math::round(mDots.y() - size / 2), size, size,
				i == mIndex ? mDotOnColor : mDotColor, 0, 0.0f, size / 2);
	}
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
