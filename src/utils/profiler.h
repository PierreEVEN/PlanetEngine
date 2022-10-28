#pragma once
#include <chrono>
#include <vector>
#include <string>
#include <mutex>
#include <thread>

#define CONCAT(x, y) x ## y
#define CONCAT_2(x, y) CONCAT(x, y)
#define STAT_FRAME(name) FrameEventRecord CONCAT_2(frame_event_recorder_, __LINE__)(name)
#define STAT_ACTION(name) ActionRecord CONCAT_2(action_recorder_, __LINE__)(name)

using TimeType = std::chrono::steady_clock::time_point;

struct Record
{
	std::string name;
	TimeType start;
	TimeType end;
	std::thread::id thread_id;
};

class Profiler
{
public:
	bool enabled = true;

	void new_frame();
	void clear_actions();

	uint64_t begin_frame_event(std::string name);
	void end_frame_event(uint64_t record);
	[[nodiscard]] double frame_event_duration(uint64_t record) const;

	uint64_t begin_action(std::string name);
	void end_action(uint64_t record);
	[[nodiscard]] double action_duration(uint64_t record);

	static Profiler& get();

	[[nodiscard]] const std::vector<Record>& get_last_frame() const { return last_frame; }
	[[nodiscard]] const std::vector<Record>& get_actions() const { return actions; }

private:
	Profiler() = default;

	std::vector<Record> frame_events;
	std::vector<Record> actions;
	std::vector<Record> last_frame;
	std::mutex action_mutex;
};

class FrameEventRecord final
{
public:
	FrameEventRecord(std::string in_stat_name);
	~FrameEventRecord();

	[[nodiscard]] double get_elapsed_milliseconds() const;

private:
	std::string stat_name;
	TimeType record_begin;
	int64_t self_ref;
};

class ActionRecord final
{
public:
	ActionRecord(std::string in_action_name);
	~ActionRecord();

	[[nodiscard]] double get_elapsed_milliseconds();

private:
	std::string stat_name;
	TimeType record_begin;
	int64_t self_ref;
};
