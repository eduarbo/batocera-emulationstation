#include "guis/GuiActivity.h"

#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "CanicaActivity.h"
#include "InputConfig.h"
#include "LocaleES.h"
#include "Window.h"

#define CELLS 10

using TerminalStyle::alpha;

static const char* WEEKDAYS[] = { "domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado" };
static const char* SHORT_DAYS[] = { "do", "lu", "ma", "mi", "ju", "vi", "sá" };

static std::string thousands(long value)
{
	std::string digits = std::to_string(value), out;
	for (size_t i = 0; i < digits.size(); i++)
	{
		if (i > 0 && (digits.size() - i) % 3 == 0)
			out += " ";

		out += digits[i];
	}

	return out;
}

GuiActivity::GuiActivity(Window* window) : GuiComponent(window), mBar(0), mFoot(0)
{
	float sw = (float)Renderer::getScreenWidth();
	float sh = (float)Renderer::getScreenHeight();
	setSize(sw, sh);

	mStyle = TerminalStyle::get(window);
	auto font = mStyle.font;
	auto small = Font::get(Math::max(8, (int)Math::round(font->getSize() * 0.74f)), font->getPath());

	// The whole screen, inside the console's margins
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

	auto data = CanicaActivity::summarize(7);
	time_t now = time(NULL);

	std::string since = "—";
	if (data.since > 0)
	{
		char buf[16];
		struct tm local;
		localtime_r(&data.since, &local);
		strftime(buf, sizeof(buf), "%d/%m/%Y", &local);
		since = std::string("desde ") + buf;
	}

	mText.add(small, TerminalStyle::spaced("CANICA // ACTIVIDAD"), x + pad, y + (mBar - smallLine) / 2, muted);
	mText.addRight(small, since, x + w - pad, y + (mBar - smallLine) / 2, accent);

	float left = x + pad, label = left + ch * 2;
	float half = x + w / 2 + ch;
	float right = x + w - pad;
	float ty = y + mBar + Math::round(row * 0.5f);

	mText.add(font, "$", left, ty, accent);
	mText.add(font, "canica actividad", label, ty, ink);
	ty += row;

	auto pair = [&](float at, const std::string& key, const std::string& value)
	{
		mText.add(font, key, at, ty, muted);
		mText.add(font, value, at + ch * 9, ty, ink);
	};

	pair(label, "total", data.totalSeconds > 0 ? CanicaActivity::duration(data.totalSeconds, true) : "—");
	pair(half, "partidas", thousands(data.plays));
	ty += row;
	pair(label, "juegos", thousands(data.gamesPlayed));
	pair(half, "racha", data.sessions == 0 ? "—" : std::to_string(data.streak) + (data.streak == 1 ? " día" : " días") +
		" · mejor " + std::to_string(data.bestStreak));
	ty += Math::round(row * 1.5f);

	// Two columns: the last seven days and the most played games
	mText.add(small, TerminalStyle::spaced("ÚLTIMOS 7 DÍAS"), label, ty + (line - smallLine), accent);
	mText.add(small, TerminalStyle::spaced("MÁS JUGADOS"), half, ty + (line - smallLine), accent);
	ty += row;

	long most = 1;
	for (long seconds : data.days)
		most = Math::max((int)most, (int)seconds);

	float barX = label + ch * 6;
	float cell = Math::round(ch * 0.9f);
	float timeRight = half - ch * 2;

	for (int i = 0; i < 7; i++)
	{
		float rowY = ty + i * row;
		time_t when = now - (6 - i) * 86400;
		struct tm local;
		localtime_r(&when, &local);

		mText.add(font, std::string(SHORT_DAYS[local.tm_wday]) + " " + (local.tm_mday < 10 ? "0" : "") + std::to_string(local.tm_mday),
			label, rowY, i == 6 ? ink : muted);

		long seconds = data.days[i];
		int full = seconds > 0 ? Math::max(1, (int)Math::round(CELLS * seconds / (float)most)) : 0;
		mBars.push_back({ barX, rowY + Math::round(line * 0.22f), cell, Math::round(line * 0.62f), full });
		mText.addRight(font, seconds > 0 ? CanicaActivity::duration(seconds) : "·", timeRight, rowY, seconds > 0 ? ink : muted);

		if (i < (int)data.top.size())
		{
			auto& game = data.top[i];
			std::string time = CanicaActivity::duration(game.seconds);
			float timeWidth = font->sizeText(time).x();

			mText.add(font, std::to_string(i + 1), half, rowY, accent);
			mText.add(font, TerminalStyle::fit(font, game.name, right - (half + ch * 2) - timeWidth - ch), half + ch * 2, rowY, ink);
			mText.addRight(font, time, right, rowY, muted);
		}
	}

	ty += 7 * row + Math::round(row * 0.5f);

	if (data.favoriteHour >= 0)
	{
		pair(label, "hora", (data.favoriteHour < 10 ? "0" : "") + std::to_string(data.favoriteHour) + ":00");
		pair(half, "día", WEEKDAYS[data.favoriteWeekday]);
	}
	else if (data.top.empty())
		mText.add(font, "aún no hay partidas", label, ty, muted);

	std::string footer = data.sessions == 0 ? "LOS DÍAS SE CUENTAN DESDE HOY" :
		thousands(data.sessions) + (data.sessions == 1 ? " SESIÓN REGISTRADA" : " SESIONES REGISTRADAS");
	mText.add(small, TerminalStyle::spaced(footer), x + pad, y + h - mFoot + (mFoot - smallLine) / 2, accent);
}

bool GuiActivity::input(InputConfig* config, Input input)
{
	if (input.value != 0 && (config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("start", input)))
	{
		delete this;
		return true;
	}

	return true;
}

void GuiActivity::render(const Transform4x4f& /*parentTrans*/)
{
	TerminalStyle::drawFrame(mStyle, mBox.x(), mBox.y(), mBox.z(), mBox.w(), mBar, mFoot);

	for (auto& bar : mBars)
		for (int i = 0; i < CELLS; i++)
			Renderer::drawRect(bar.x + i * bar.cell, bar.y, Math::max(1.0f, bar.cell - 2), bar.height,
				i < bar.full ? mStyle.fillColor : mStyle.trackColor);

	mText.render();
	TerminalStyle::drawScanLines();
}

std::vector<HelpPrompt> GuiActivity::getHelpPrompts()
{
	return { HelpPrompt(BUTTON_BACK, _("BACK")) };
}
