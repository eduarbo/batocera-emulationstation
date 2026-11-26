#pragma once
#ifndef ES_CORE_PLATFORM_H
#define ES_CORE_PLATFORM_H

#include <cstdio>
#include <string>

#ifdef WIN32
#include <Windows.h>
#include <intrin.h>

#define sleep Sleep
#endif

class Window;

namespace Utils
{
	namespace Platform
	{
		enum QuitMode
		{
			QUIT = 0,
			RESTART = 1,
			SHUTDOWN = 2,
			REBOOT = 3,
			FAST_SHUTDOWN = 4,
			FAST_REBOOT = 5
		};

		class ProcessStartInfo
		{
		public:
			ProcessStartInfo();
			ProcessStartInfo(const std::string& cmd);

			int run() const;

#ifndef WIN32
			// run() in two steps, for a command that should start before its caller is done with
			// something else: start() launches it (false if it could not), wait() reads its
			// output as run() does and returns its exit code.
			bool start();
			int wait();
#endif

			std::string command;
			bool waitForExit;
			bool showWindow;
			Window* window;
#ifndef WIN32
			std::string stderrFilename;
			std::string stdoutFilename;

		private:
			FILE* mChild = nullptr;
			std::string mOutput;
#endif
		};

		int quitES(QuitMode mode = QuitMode::QUIT);
		bool isFastShutdown();
		void processQuitMode();

		struct BatteryInformation
		{
			BatteryInformation()
			{
				hasBattery = false;
				level = 0;
				isCharging = false;
			}

			bool hasBattery;
			int  level;
			bool isCharging;
		};

		BatteryInformation queryBatteryInformation();

		std::string queryIPAdress();
		std::string getArchString();

#if WIN32
		bool isWindows11();
#endif
	}
}

#endif // ES_CORE_PLATFORM_H
