#include "CanicaUpdates.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#define FOLDER std::string("/userdata/system/canica-updates")

// A reader never sees half a file: written beside it, then renamed over it
static void replace(const std::string& name, const std::string& text)
{
	if (!Utils::FileSystem::exists(FOLDER))
		return;

	Utils::FileSystem::writeAllText(FOLDER + "/" + name + ".tmp", text + "\n");
	Utils::FileSystem::renameFile(FOLDER + "/" + name + ".tmp", FOLDER + "/" + name);
}

std::map<std::string, std::string> CanicaUpdates::status()
{
	std::map<std::string, std::string> values;

	if (!Utils::FileSystem::exists(FOLDER + "/status"))
		return values;

	for (const std::string& row : Utils::String::split(Utils::FileSystem::readAllText(FOLDER + "/status"), '\n', true))
	{
		size_t equals = row.find('=');
		if (equals != std::string::npos && equals > 0)
			values[row.substr(0, equals)] = Utils::String::trim(row.substr(equals + 1));
	}

	return values;
}

std::string CanicaUpdates::mode()
{
	std::string mode = Utils::FileSystem::exists(FOLDER + "/mode") ? Utils::String::trim(Utils::FileSystem::readAllText(FOLDER + "/mode")) : "";
	return mode == "auto" || mode == "off" ? mode : "ask";
}

void CanicaUpdates::setMode(const std::string& mode)
{
	if (mode == "ask" || mode == "auto" || mode == "off")
		replace("mode", mode);
}

void CanicaUpdates::request(const std::string& line)
{
	replace("request", line);
}
