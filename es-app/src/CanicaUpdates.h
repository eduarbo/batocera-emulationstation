#pragma once
#ifndef ES_APP_CANICA_UPDATES_H
#define ES_APP_CANICA_UPDATES_H

#include <map>
#include <string>

// The menu's side of the kit updater, a program outside the frontend. They talk through small files in one
// folder: the menu reads what the updater found, and leaves it requests. The menu runs no installer itself.
namespace CanicaUpdates
{
	// The kit is on this card
	bool kitInstalled();

	// What the updater last wrote: installed, available, serial, ready, detail. Empty without an updater here.
	std::map<std::string, std::string> status();

	// ask (before installing), auto or off
	std::string mode();
	void setMode(const std::string& mode);

	// One line for the updater: check, mode, install <serial> or later <serial>. A newer one replaces the last.
	void request(const std::string& line);
}

#endif // ES_APP_CANICA_UPDATES_H
