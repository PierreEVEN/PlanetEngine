#include "profiler.h"
#include <memory>

void Profiler::new_frame()
{
	last_frame = std::move(records);
	records = std::vector<Record>();
}

Record* Profiler::add_record(const char* name, const TimeType& start, const TimeType& end)
{
	if (!enabled)
		return nullptr;

	records.emplace_back(Record{name, start, end});
	return &records[records.size() - 1];
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
	  self_ref(Profiler::get().add_record(stat_name, std::chrono::steady_clock::now(),
	                                      std::chrono::steady_clock::now()))
{
}

TimeWatcher::~TimeWatcher()
{
	if (self_ref)
		self_ref->end = std::chrono::steady_clock::now();
}
