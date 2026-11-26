#include "guis/GuiActivity.h"

#include "components/TextComponent.h"
#include "guis/GuiSettings.h"
#include "CanicaActivity.h"
#include "CanicaText.h"
#include "ThemeData.h"
#include "Window.h"

static const char* WEEKDAYS[] = { "DOMINGO", "LUNES", "MARTES", "MIÉRCOLES", "JUEVES", "VIERNES", "SÁBADO" };
static const char* WEEKDAY_NAMES[] = { "domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado" };
static const char* WEEKDAYS_EN[] = { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY" };
static const char* WEEKDAY_NAMES_EN[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// 12 345 in Spanish, 12,345 in English
static std::string thousands(long value)
{
	const std::string separator = Canica::spanish() ? " " : ",";
	std::string digits = std::to_string(value), out;
	for (size_t i = 0; i < digits.size(); i++)
	{
		if (i > 0 && (digits.size() - i) % 3 == 0)
			out += separator;

		out += digits[i];
	}

	return out;
}

void GuiActivity::open(Window* window)
{
	auto data = CanicaActivity::summarize(7);
	auto theme = ThemeData::getMenuTheme();
	const bool es = Canica::spanish();
	GuiSettings* s = new GuiSettings(window, es ? "ACTIVIDAD" : "ACTIVITY");

	auto value = [window, theme](const std::string& text)
	{
		return std::make_shared<TextComponent>(window, text, theme->Text.font, theme->Text.color, ALIGN_RIGHT);
	};

	s->addGroup(es ? "RESUMEN" : "SUMMARY");
	s->addWithLabel(es ? "TIEMPO DE JUEGO" : "PLAY TIME", value(data.totalSeconds > 0 ? CanicaActivity::duration(data.totalSeconds, true) : "—"));
	s->addWithLabel(es ? "JUEGOS JUGADOS" : "GAMES PLAYED", value(thousands(data.gamesPlayed)));
	s->addWithLabel(es ? "PARTIDAS" : "PLAYS", value(thousands(data.plays)));

	if (data.bestStreak > 0)
	{
		const char* days = es ? (data.streak == 1 ? " día" : " días") : (data.streak == 1 ? " day" : " days");
		s->addWithLabel(es ? "RACHA" : "STREAK", value(std::to_string(data.streak) + days +
			(es ? " · mejor " : " · best ") + std::to_string(data.bestStreak)));
	}

	if (data.sessions > 0)
	{
		s->addWithLabel(es ? "HORA FAVORITA" : "FAVORITE HOUR", value((data.favoriteHour < 10 ? "0" : "") + std::to_string(data.favoriteHour) + ":00"));
		s->addWithLabel(es ? "DÍA FAVORITO" : "FAVORITE DAY", value((es ? WEEKDAY_NAMES : WEEKDAY_NAMES_EN)[data.favoriteWeekday]));
	}

	// Today first; the log starts with this version, so a new console says so
	s->addGroup(es ? (data.sessions > 0 ? "ÚLTIMOS 7 DÍAS" : "ÚLTIMOS 7 DÍAS · SE CUENTAN DESDE HOY")
		: (data.sessions > 0 ? "LAST 7 DAYS" : "LAST 7 DAYS · COUNTED FROM TODAY"));
	time_t now = time(NULL);
	for (int i = 6; i >= 0; i--)
	{
		time_t when = now - (6 - i) * 86400;
		struct tm local;
		localtime_r(&when, &local);

		std::string day = i == 6 ? (es ? "HOY" : "TODAY") : (i == 5 ? (es ? "AYER" : "YESTERDAY") :
			std::string((es ? WEEKDAYS : WEEKDAYS_EN)[local.tm_wday]) + " " + std::to_string(local.tm_mday));
		s->addWithLabel(day, value(data.days[i] > 0 ? CanicaActivity::duration(data.days[i]) : "—"));
	}

	if (!data.top.empty())
	{
		s->addGroup(es ? "MÁS JUGADOS" : "MOST PLAYED");
		for (size_t i = 0; i < data.top.size(); i++)
			s->addWithLabel(std::to_string(i + 1) + ". " + data.top[i].name, value(CanicaActivity::duration(data.top[i].seconds)));
	}

	window->pushGui(s);
}
