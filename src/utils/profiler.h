#pragma once
#include <chrono>

#define CONCAT(x, y) x ## y
#define CONCAT_2(x, y) CONCAT(x, y)
#define STAT_DURATION(name) TimeWatcher CONCAT_2(time_watcher, __LINE__)(#name)

using TimeType = std::chrono::steady_clock::time_point;

struct Record
{
	const char* name;
	TimeType start;
	TimeType end;
};

class Profiler
{
public:

	void new_frame();
	Record* add_record(const char* name, const TimeType& start, const TimeType& end);

	bool enabled = true;

	static Profiler& get();

	[[nodiscard]] const std::vector<Record>& get_last_frame() const { return last_frame; }

private:
	Profiler() = default;

	std::vector<Record> records;
	std::vector<Record> last_frame;
};

class TimeWatcher final
{
public:
	TimeWatcher(const char* in_stat_name);
	~TimeWatcher();

private:
	const char* stat_name;
	TimeType record_begin;
	Record* self_ref;
};
