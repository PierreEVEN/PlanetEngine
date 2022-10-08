#pragma once
#include <chrono>
#include <vector>

#define CONCAT(x, y) x ## y
#define CONCAT_2(x, y) CONCAT(x, y)
#define STAT_DURATION(name) TimeWatcher CONCAT_2(time_watcher, __LINE__)(name)

using TimeType = std::chrono::steady_clock::time_point;

struct Record
{
	std::string name;
	TimeType start;
	TimeType end;
};

class Profiler
{
public:

	void new_frame();
	uint64_t add_record(std::string name);
	void close_record(uint64_t record);

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
	TimeWatcher(std::string in_stat_name);
	~TimeWatcher();

private:
	std::string stat_name;
	TimeType record_begin;
	int64_t self_ref;
};
