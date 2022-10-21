#include "profiler.h"

#include <memory>
#include <iostream>
#include <memory>

std::unique_ptr<Profiler> profiler_singleton = nullptr;

Profiler& Profiler::get()
{
	if (!profiler_singleton)
		profiler_singleton = std::unique_ptr<Profiler>(new Profiler());

	return *profiler_singleton;
}

void Profiler::new_frame()
{
	last_frame = std::move(frame_events);
	frame_events = std::vector<Record>();
}

void Profiler::clear_actions()
{
	actions.clear();
}

uint64_t Profiler::begin_frame_event(std::string name)
{
	if (!enabled)
		return -1;

	frame_events.emplace_back(Record {std::move(name), std::chrono::steady_clock::now(), std::chrono::steady_clock::now(), std::this_thread::get_id()} );
	return frame_events.size() - 1;
}

void Profiler::end_frame_event(uint64_t record)
{
	frame_events[record].end = std::chrono::steady_clock::now();
}

double Profiler::frame_event_duration(uint64_t record) const
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - frame_events[record].start).count() / 1000.0;
}

uint64_t Profiler::begin_action(std::string name)
{
	std::lock_guard lock_guard(action_mutex);
	if (!enabled)
		return -1;

	actions.emplace_back(Record{ std::move(name), std::chrono::steady_clock::now(), std::chrono::steady_clock::now(), std::this_thread::get_id() });
	return actions.size() - 1;
}

void Profiler::end_action(uint64_t record)
{
	std::lock_guard lock_guard(action_mutex);
	actions[record].end = std::chrono::steady_clock::now();
}

double Profiler::action_duration(uint64_t record)
{
	std::lock_guard lock_guard(action_mutex);
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - actions[record].start).count() / 1000.0;
}

FrameEventRecord::FrameEventRecord(std::string in_stat_name)
	: stat_name(in_stat_name), record_begin(std::chrono::steady_clock::now()),
	  self_ref(-1)
{
	self_ref = Profiler::get().begin_frame_event(std::move(stat_name));
}

FrameEventRecord::~FrameEventRecord()
{
	if (self_ref >= 0)
		Profiler::get().end_frame_event(self_ref);
}

double FrameEventRecord::get_elapsed_milliseconds() const
{
	if (self_ref >= 0)
		return Profiler::get().frame_event_duration(self_ref);
	return -1;
}

ActionRecord::ActionRecord(std::string in_action_name)
	: stat_name(in_action_name), record_begin(std::chrono::steady_clock::now()),
	self_ref(-1)
{
	self_ref = Profiler::get().begin_action(std::move(stat_name));
}

ActionRecord::~ActionRecord()
{
	if (self_ref >= 0)
		Profiler::get().end_action(self_ref);
}

double ActionRecord::get_elapsed_milliseconds()
{
	if (self_ref >= 0)
		return Profiler::get().frame_event_duration(self_ref);
	return -1;
}
