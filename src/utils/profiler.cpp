#include "profiler.h"

void Profiler::new_frame()
{
	last_frame = std::move(records);
	records = std::vector<Record>();
}

void Profiler::add_record(const char* name, const TimeType& start, const TimeType& end)
{
	if (!enabled)
		return;

	records.emplace_back(name, start, end);
}

std::unique_ptr<Profiler> profiler_singleton = nullptr;

Profiler& Profiler::get()
{
	if (!profiler_singleton)
		profiler_singleton = std::unique_ptr<Profiler>(new Profiler());

	return *profiler_singleton;
}

TimeWatcher::TimeWatcher(const char* in_stat_name)
	: stat_name(in_stat_name), record_begin(std::chrono::steady_clock::now())
{
}

TimeWatcher::~TimeWatcher()
{
	const auto now = std::chrono::steady_clock::now();
	Profiler::get().add_record(stat_name, record_begin, now);
}
