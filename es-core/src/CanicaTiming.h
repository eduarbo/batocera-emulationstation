#pragma once
#ifndef ES_CORE_CANICA_TIMING_H
#define ES_CORE_CANICA_TIMING_H

#include <chrono>
#include <cstdio>
#include <unistd.h>

// The steps of a game launch and of the way back, one line each (wall clock, milliseconds), in
// /var/run/canica-launch-timing.log, and only while that file exists: create it to measure.
inline void canicaTiming(const char* step)
{
	static const char* path = "/var/run/canica-launch-timing.log";
	if (access(path, W_OK) != 0)
		return;
	FILE* file = fopen(path, "a");
	if (file == nullptr)
		return;
	long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	fprintf(file, "%lld.%03lld %s\n", ms / 1000, ms % 1000, step);
	fclose(file);
}

#endif // ES_CORE_CANICA_TIMING_H
