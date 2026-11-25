#include "CanicaActivity.h"

#include "math/Misc.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "SystemData.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <set>

#define FOLDER std::string("/userdata/system/canica-activity")
#define LOG_FILE (FOLDER + "/sessions.tsv")
#define MIN_SESSION 60	// shorter sessions do not make a day of play

// The local calendar day of a moment, as a number
static long localDay(time_t when)
{
	struct tm local;
	localtime_r(&when, &local);
	long shifted = (long)when + local.tm_gmtoff;
	return shifted >= 0 ? shifted / 86400 : (shifted - 86399) / 86400;
}

static std::string clean(const std::string& text)
{
	std::string out = text;
	for (char& c : out)
		if (c == '\t' || c == '\n' || c == '\r')
			c = ' ';

	return out;
}

void CanicaActivity::record(FileData* game, time_t start, long seconds)
{
	if (game == nullptr || seconds <= 0 || !Utils::FileSystem::exists("/userdata/system"))
		return;

	if (!Utils::FileSystem::exists(FOLDER))
		Utils::FileSystem::createDirectory(FOLDER);

	std::ofstream log(LOG_FILE, std::ios::app);
	if (!log.is_open())
		return;

	std::string system = game->getSourceFileData()->getSystem() ? game->getSourceFileData()->getSystem()->getName() : "";
	log << (long long)start << '\t' << seconds << '\t' << clean(system) << '\t' << clean(game->getPath()) << '\n';
}

CanicaActivity::Summary CanicaActivity::summarize(size_t topCount)
{
	Summary summary;

	// Every game the menu knows, once
	for (auto system : SystemData::sSystemVector)
	{
		if (system->isCollection() || system->isGroupSystem() || SystemData::isUtilitySystem(system) || system->getRootFolder() == nullptr)
			continue;

		for (auto game : system->getRootFolder()->getFilesRecursive(GAME))
		{
			long seconds = atol(game->getMetadata(MetaDataId::GameTime).c_str());
			int plays = Utils::String::toInteger(game->getMetadata(MetaDataId::PlayCount));
			if (seconds <= 0 && plays <= 0)
				continue;

			summary.gamesPlayed++;
			summary.plays += Math::max(0, plays);

			if (seconds > 0)
			{
				summary.totalSeconds += seconds;
				summary.top.push_back({ game->getName(), seconds });
			}
		}
	}

	std::sort(summary.top.begin(), summary.top.end(), [](const Game& a, const Game& b) { return a.seconds > b.seconds; });
	if (summary.top.size() > topCount)
		summary.top.resize(topCount);

	// The log: days, streaks, hours
	std::ifstream log(LOG_FILE);
	if (!log.is_open())
		return summary;

	long today = localDay(time(NULL));
	std::set<long> played;
	long hours[24] = {}, weekdays[7] = {};
	std::string line;

	while (std::getline(log, line))
	{
		std::vector<std::string> cols = Utils::String::split(line, '\t');
		if (cols.size() < 2)
			continue;

		time_t start = (time_t)atoll(cols[0].c_str());
		long seconds = atol(cols[1].c_str());
		if (start <= 0 || seconds <= 0)
			continue;

		summary.sessions++;
		if (summary.since == 0 || start < summary.since)
			summary.since = start;

		long day = localDay(start);
		if (day <= today && today - day < 7)
			summary.days[6 - (today - day)] += seconds;

		if (seconds >= MIN_SESSION)
			played.insert(day);

		struct tm local;
		localtime_r(&start, &local);
		hours[local.tm_hour] += seconds;
		weekdays[local.tm_wday] += seconds;
	}

	if (summary.sessions == 0)
		return summary;

	summary.favoriteHour = (int)(std::max_element(hours, hours + 24) - hours);
	summary.favoriteWeekday = (int)(std::max_element(weekdays, weekdays + 7) - weekdays);

	// The current streak counts from today, or from yesterday when today has nothing yet
	long day = played.count(today) ? today : today - 1;
	while (played.count(day))
	{
		summary.streak++;
		day--;
	}

	int run = 0;
	long previous = 0;
	for (long played_day : played)
	{
		run = (run > 0 && played_day == previous + 1) ? run + 1 : 1;
		previous = played_day;
		summary.bestStreak = Math::max(summary.bestStreak, run);
	}

	return summary;
}

std::string CanicaActivity::duration(long seconds, bool total)
{
	long minutes = seconds / 60;
	long hours = minutes / 60;
	minutes %= 60;

	if (hours == 0)
		return seconds < 60 && !total ? "<1 min" : std::to_string(minutes) + " min";

	if (total)
		return std::to_string(hours) + " h " + std::to_string(minutes) + " min";

	if (minutes == 0 || hours >= 10)
		return std::to_string(hours) + " h";

	return std::to_string(hours) + " h " + (minutes < 10 ? "0" : "") + std::to_string(minutes);
}
