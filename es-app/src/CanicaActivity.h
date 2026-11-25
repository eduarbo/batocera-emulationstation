#pragma once
#ifndef ES_APP_CANICA_ACTIVITY_H
#define ES_APP_CANICA_ACTIVITY_H

#include <string>
#include <time.h>
#include <vector>

class FileData;

// What was played and when. The time of each game is the menu's own (its gametime metadata); each session
// is also written to a log, so days, streaks and the favourite hour can be counted.
namespace CanicaActivity
{
	struct Game
	{
		std::string name;
		long seconds;
	};

	struct Summary
	{
		Summary() : totalSeconds(0), gamesPlayed(0), plays(0), streak(0), bestStreak(0), favoriteHour(-1),
			favoriteWeekday(-1), since(0), sessions(0) { for (int i = 0; i < 7; i++) days[i] = 0; }

		long totalSeconds;			// every game, since the menu counts
		int gamesPlayed;
		int plays;
		std::vector<Game> top;		// the most played, longest first

		long days[7];				// from the log: seconds per local day, today last
		int streak;					// days in a row up to today (or yesterday)
		int bestStreak;
		int favoriteHour;			// 0-23, -1 without sessions
		int favoriteWeekday;		// 0 Sunday to 6 Saturday, -1 without sessions
		time_t since;				// the first logged session
		int sessions;
	};

	// After a game: when it started and how long the console was awake in it
	void record(FileData* game, time_t start, long seconds);

	Summary summarize(size_t topCount = 7);

	// "35 min", "1 h 20", "24 h"; with total, "143 h 12 min"
	std::string duration(long seconds, bool total = false);
}

#endif // ES_APP_CANICA_ACTIVITY_H
