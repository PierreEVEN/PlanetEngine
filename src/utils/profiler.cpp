#include "profiler.h"

#include <memory>
#include <iostream>

void Profiler::new_frame()
{
	last_frame = std::move(records);
	records = std::vector<Record>();
}

uint64_t Profiler::add_record(const char* name)
{
	if (!enabled)
		return -1;

	records.emplace_back(name, std::chrono::steady_clock::now(), std::chrono::steady_clock::now());
	return records.size() - 1;
}

void Profiler::close_record(uint64_t record)
{
	records[record].end = std::chrono::steady_clock::now();
}

std::unique_ptr<Profiler> profiler_singleton = nullptr;

Profiler& Profiler::get()
{
	if (!profiler_singleton)
		profiler_singleton = std::unique_ptr<Profiler>(new Profiler());

	return *profiler_singleton;
}

TimeWatcher::TimeWatcher(const char* in_stat_name)
	: stat_name(in_stat_name), record_begin(std::chrono::steady_clock::now()),
	  self_ref(-1)
{
	self_ref = Profiler::get().add_record(stat_name);
}

TimeWatcher::~TimeWatcher()
{
	if (self_ref >= 0)
		Profiler::get().close_record(self_ref);
}
