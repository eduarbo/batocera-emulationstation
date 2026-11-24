#include "guis/GuiJobConsole.h"

#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "InputConfig.h"
#include "LocaleES.h"
#include "PowerSaver.h"
#include "Window.h"

#define CELLS 40
#define QUIET_LIMIT 180000		// a job that says nothing for this long is gone: the menu comes back
#define TAKEOVER_LIMIT 45000	// and so is one that does not take over after a yes

GuiJobConsole* GuiJobConsole::sInstance = nullptr;

static unsigned int alpha(unsigned int color, unsigned int a)
{
	return (color & 0xFFFFFF00) | (((color & 0xFF) * a) / 255);
}

// Letters set apart, as on the title bar of an old terminal
static std::string spaced(const std::string& text)
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

void GuiJobConsole::show(Window* window, const Screen& screen, const std::function<void(int)>& onAnswer)
{
	if (sInstance == nullptr)
	{
		window->cancelScreenSaver();
		sInstance = new GuiJobConsole(window);
		window->pushGui(sInstance);
	}

	sInstance->set(screen, onAnswer);
}

void GuiJobConsole::close()
{
	if (sInstance != nullptr)
		delete sInstance;
}

GuiJobConsole::GuiJobConsole(Window* window) : GuiComponent(window),
	mFocus(0), mQuiet(0), mQuietLimit(QUIET_LIMIT), mBlink(0), mBar(0), mFoot(0)
{
	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	PowerSaver::pause();	// the cursor blinks and a silent job is timed
}

GuiJobConsole::~GuiJobConsole()
{
	clear();
	PowerSaver::resume();

	if (sInstance == this)
		sInstance = nullptr;
}

void GuiJobConsole::set(const Screen& screen, const std::function<void(int)>& onAnswer)
{
	mScreen = screen;
	mOnAnswer = onAnswer;
	mFocus = 0;
	mQuiet = 0;
	mQuietLimit = QUIET_LIMIT;

	layout();
	updateHelpPrompts();
}

void GuiJobConsole::clear()
{
	for (auto& text : mTexts)
		delete text.cache;

	mTexts.clear();
}

void GuiJobConsole::add(const std::shared_ptr<Font>& font, const std::string& text, float x, float y, unsigned int color)
{
	if (!text.empty())
		mTexts.push_back({ font, font->buildTextCache(text, Math::round(x), Math::round(y), color) });
}

void GuiJobConsole::layout()
{
	clear();
	mCursor = mGauge = mButtons[0] = mButtons[1] = Vector4f(0, 0, 0, 0);

	const Window::PopupStyle& style = mWindow->getConsoleStyle();
	if (!style.enabled || style.font == nullptr)
		return;

	mFont = style.font;
	mSmallFont = Font::get(Math::max(8, (int)Math::round(mFont->getSize() * 0.74f)), mFont->getPath());

	float x = Math::round(style.pos.x() - style.size.x() * style.origin.x());
	float y = Math::round(style.pos.y() - style.size.y() * style.origin.y());
	float w = style.size.x();
	float h = style.size.y();
	mBox = Vector4f(x, y, w, h);

	float ch = mFont->sizeText("0").x();	// one cell of the monospaced grid
	float line = mFont->getHeight(1.0f);
	float row = Math::round(line * 1.5f);
	float small = mSmallFont->getHeight(1.0f);
	float pad = Math::round(ch * 2);
	mBar = Math::round(small * 2.3f);
	mFoot = Math::round(small * 2.6f);

	unsigned int ink = style.color;
	unsigned int muted = alpha(ink, 150);
	unsigned int accent = style.fillColor;

	// the title bar: what this is, and from which version to which
	add(mSmallFont, spaced(mScreen.title), x + pad, y + (mBar - small) / 2, muted);
	add(mSmallFont, mScreen.tag, x + w - pad - mSmallFont->sizeText(mScreen.tag).x(), y + (mBar - small) / 2, accent);

	// the log: a prompt, the steps done and the one under way with its cursor
	float label = x + pad + ch * 2;
	float value = label + ch * 12;
	float ty = y + mBar + Math::round(row * 0.6f);

	for (const std::string& text : mScreen.lines)
	{
		std::vector<std::string> cols = Utils::String::split(text, '\t');

		if (cols.size() < 2 && Utils::String::startsWith(text, "$ "))
		{
			add(mFont, "$", x + pad, ty, accent);
			add(mFont, text.substr(2), label, ty, ink);
		}
		else if (cols.size() < 2)
			add(mFont, text, label, ty, muted);
		else if (cols.size() > 2)
		{
			add(mFont, cols[0], label, ty, muted);
			add(mFont, cols[1], value, ty, alpha(ink, 215));
			add(mFont, cols[2], x + w - pad - mFont->sizeText(cols[2]).x(), ty, accent);
		}
		else
		{
			add(mFont, ">", x + pad, ty, accent);
			add(mFont, cols[0], label, ty, ink);
			add(mFont, cols[1], value, ty, ink);
			mCursor = Vector4f(Math::round(value + mFont->sizeText(cols[1]).x() + ch * 0.6f), ty + Math::round(line * 0.1f), Math::round(ch), Math::round(line * 0.86f));
		}

		ty += row;
	}

	// above the footer: the two answers of a question, or the gauge
	float gy = y + h - mFoot - Math::round(row * 1.7f);

	if (mScreen.answers.size() >= 2)
	{
		float bx = label;
		float bh = Math::round(row * 1.3f);

		for (int i = 0; i < 2; i++)
		{
			float bw = Math::round(mFont->sizeText(mScreen.answers[i]).x() + ch * 4);
			mButtons[i] = Vector4f(bx, gy - Math::round(row * 0.3f), bw, bh);
			add(mFont, mScreen.answers[i], bx + ch * 2, gy - Math::round(row * 0.3f) + (bh - line) / 2, i == mFocus ? style.backgroundColor | 0xFF : accent);
			bx += bw + ch * 2;
		}
	}
	else if (mScreen.percent >= 0)
	{
		std::string percent = std::to_string(Math::min(mScreen.percent, 100)) + " %";
		float cell = (float)(int)((w - pad * 2 - ch * 2 - ch * 7) / CELLS);

		mGauge = Vector4f(label, gy, Math::max(3.0f, cell), Math::round(row * 0.6f));
		add(mFont, percent, x + w - pad - mFont->sizeText(percent).x(), gy - (line - row * 0.6f) / 2, ink);
	}

	add(mSmallFont, spaced(mScreen.footer), x + pad, y + h - mFoot + (mFoot - small) / 2, accent);
}

bool GuiJobConsole::input(InputConfig* config, Input input)
{
	if (mScreen.answers.size() >= 2 && input.value != 0)
	{
		if (config->isMappedLike("left", input) || config->isMappedLike("right", input))
		{
			mFocus = 1 - mFocus;
			layout();
		}
		else if (config->isMappedTo(BUTTON_OK, input))
			answer(mFocus);
		else if (config->isMappedTo(BUTTON_BACK, input))
			answer(0);
	}

	return true;	// nothing under this screen may start while the job works
}

void GuiJobConsole::answer(int index)
{
	auto told = mOnAnswer;

	if (index == 0)
	{
		delete this;

		if (told)
			told(0);

		return;
	}

	// The job takes over from here: its next word replaces this screen
	Screen next = mScreen;
	if (mScreen.answers.size() > 2)
	{
		std::vector<std::string> row(mScreen.answers.begin() + 2, mScreen.answers.end());
		next.lines.push_back(Utils::String::join(row, "\t"));
	}

	next.answers.clear();
	set(next, nullptr);
	mQuietLimit = TAKEOVER_LIMIT;

	if (told)
		told(1);
}

void GuiJobConsole::update(int deltaTime)
{
	mBlink += deltaTime;
	mQuiet += deltaTime;

	// A question waits for its answer. A job that went silent does not keep the menu.
	if (mScreen.answers.size() < 2 && mQuiet > mQuietLimit)
	{
		mQuiet = 0;
		mWindow->postToUiThread([] { GuiJobConsole::close(); });
	}
}

void GuiJobConsole::render(const Transform4x4f& /*parentTrans*/)
{
	const Window::PopupStyle& style = mWindow->getConsoleStyle();
	if (!style.enabled || mFont == nullptr)
		return;

	float sw = (float)Renderer::getScreenWidth();
	float sh = (float)Renderer::getScreenHeight();
	float x = mBox.x(), y = mBox.y(), w = mBox.z(), h = mBox.w();
	unsigned int accent = style.fillColor;

	Renderer::setMatrix(Transform4x4f::Identity());
	Renderer::drawRect(0.0f, 0.0f, sw, sh, style.trackBorderColor);

	// the glow of a phosphor screen, then the window with its two rules
	for (int i = 3; i > 0; i--)
		Renderer::drawSolidRectangle(x - i * 2, y - i * 2, w + i * 4, h + i * 4, 0, alpha(accent, 22 * (4 - i)), 2.0f, style.cornerRadius + i * 2);

	Renderer::drawSolidRectangle(x, y, w, h, style.backgroundColor, style.borderColor, 1.0f, style.cornerRadius);
	Renderer::drawRect(x + 1, y + mBar, w - 2, 1.0f, alpha(accent, 60));
	Renderer::drawRect(x + 1, y + h - mFoot, w - 2, 1.0f, alpha(accent, 60));

	if (mGauge.z() > 0)
	{
		int full = (int)Math::round(CELLS * Math::min(mScreen.percent, 100) / 100.0f);

		for (int i = 0; i < CELLS; i++)
			Renderer::drawRect(mGauge.x() + i * mGauge.z(), mGauge.y(), Math::max(1.0f, mGauge.z() - 3), mGauge.w(), i < full ? accent : style.trackColor);
	}

	for (int i = 0; i < 2; i++)
		if (mButtons[i].z() > 0)
			Renderer::drawSolidRectangle(mButtons[i].x(), mButtons[i].y(), mButtons[i].z(), mButtons[i].w(),
				i == mFocus ? accent : 0, i == mFocus ? accent : alpha(accent, 150), 1.0f, 2.0f);

	if (mCursor.z() > 0 && (mBlink / 530) % 2 == 0)
		Renderer::drawRect(mCursor.x(), mCursor.y(), mCursor.z(), mCursor.w(), accent);

	for (auto& text : mTexts)
		text.font->renderTextCache(text.cache);

	// scan lines over everything, barely there
	for (float line = 0; line < sh; line += 3)
		Renderer::drawRect(0.0f, line, sw, 1.0f, 0x00000022);
}

std::vector<HelpPrompt> GuiJobConsole::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if (mScreen.answers.size() >= 2)
	{
		prompts.push_back(HelpPrompt("left/right", _("CHOOSE")));
		prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT")));
		prompts.push_back(HelpPrompt(BUTTON_BACK, mScreen.answers[0]));
	}

	return prompts;
}
