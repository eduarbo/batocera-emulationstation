#pragma once
#ifndef ES_CORE_CANICA_TEXT_H
#define ES_CORE_CANICA_TEXT_H

#include "SystemConf.h"
#include <string>

// Canica's own words, which the firmware's translation catalogues cannot carry: Spanish on a console whose
// language is Spanish (system.language es or es_*), English on any other. The stock menus keep gettext.
namespace Canica
{
	inline bool spanish()
	{
		const std::string language = SystemConf::getInstance()->get("system.language");
		return language.size() >= 2 && (language[0] == 'e' || language[0] == 'E') && (language[1] == 's' || language[1] == 'S') &&
			(language.size() == 2 || language[2] == '_' || language[2] == '-' || language[2] == '.');
	}

	inline std::string text(const char* es, const char* en)
	{
		return spanish() ? es : en;
	}
}

#endif // ES_CORE_CANICA_TEXT_H
