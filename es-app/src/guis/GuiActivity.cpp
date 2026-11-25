#include "guis/GuiActivity.h"

#include "components/TextComponent.h"
#include "guis/GuiSettings.h"
#include "CanicaActivity.h"
#include "ThemeData.h"
#include "Window.h"

static const char* WEEKDAYS[] = { "DOMINGO", "LUNES", "MARTES", "MIÉRCOLES", "JUEVES", "VIERNES", "SÁBADO" };
static const char* WEEKDAY_NAMES[] = { "domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado" };

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

void GuiActivity::open(Window* window)
{
	auto data = CanicaActivity::summarize(7);
	auto theme = ThemeData::getMenuTheme();
	GuiSettings* s = new GuiSettings(window, "ACTIVIDAD");

	auto value = [window, theme](const std::string& text)
	{
		return std::make_shared<TextComponent>(window, text, theme->Text.font, theme->Text.color, ALIGN_RIGHT);
	};

	s->addGroup("RESUMEN");
	s->addWithLabel("TIEMPO DE JUEGO", value(data.totalSeconds > 0 ? CanicaActivity::duration(data.totalSeconds, true) : "—"));
	s->addWithLabel("JUEGOS JUGADOS", value(thousands(data.gamesPlayed)));
	s->addWithLabel("PARTIDAS", value(thousands(data.plays)));

	if (data.bestStreak > 0)
		s->addWithLabel("RACHA", value(std::to_string(data.streak) + (data.streak == 1 ? " día" : " días") +
			" · mejor " + std::to_string(data.bestStreak)));

	if (data.sessions > 0)
	{
		s->addWithLabel("HORA FAVORITA", value((data.favoriteHour < 10 ? "0" : "") + std::to_string(data.favoriteHour) + ":00"));
		s->addWithLabel("DÍA FAVORITO", value(WEEKDAY_NAMES[data.favoriteWeekday]));
	}

	// Today first; the log starts with this version, so a new console says so
	s->addGroup(data.sessions > 0 ? "ÚLTIMOS 7 DÍAS" : "ÚLTIMOS 7 DÍAS · SE CUENTAN DESDE HOY");
	time_t now = time(NULL);
	for (int i = 6; i >= 0; i--)
	{
		time_t when = now - (6 - i) * 86400;
		struct tm local;
		localtime_r(&when, &local);

		std::string day = i == 6 ? "HOY" : (i == 5 ? "AYER" : std::string(WEEKDAYS[local.tm_wday]) + " " + std::to_string(local.tm_mday));
		s->addWithLabel(day, value(data.days[i] > 0 ? CanicaActivity::duration(data.days[i]) : "—"));
	}

	if (!data.top.empty())
	{
		s->addGroup("MÁS JUGADOS");
		for (size_t i = 0; i < data.top.size(); i++)
			s->addWithLabel(std::to_string(i + 1) + ". " + data.top[i].name, value(CanicaActivity::duration(data.top[i].seconds)));
	}

	window->pushGui(s);
}
