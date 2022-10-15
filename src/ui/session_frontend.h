#pragma once
#include <vector>

#include "ui.h"
#include "utils/profiler.h"

struct DrawRecord;
struct Record;

class SessionFrontend final : public ImGuiWindow
{
public:
	SessionFrontend();
	~SessionFrontend() override;

	void draw() override;
private:


	struct RecordData
	{
		std::vector<std::vector<DrawRecord>> lines;
		float min_value = -10000;
		float max_value = -10000;

		std::vector<DrawRecord>& get_line(int index);
	};

	void compute_record_data(RecordData& output, int level, std::vector<Record>::iterator& it,
		const std::vector<Record>& records, const TimeType& parent_end);
	void draw_record_data(const RecordData& data) const;
	void draw_record(const DrawRecord& record, int line_index, float scale, int index, size_t line_count) const;

	void update_draw_delta_seconds();

	std::vector<Record> last_frame;
	RecordData data;
	float zoom = 0;
	bool realtime = false;
	float custom_width = 0;
	bool use_custom_width = false;
	float recorded_max = 0;

	int record_first_frame = 5;

	std::vector<float> delta_time_history;
	int delta_time_index = 0;
	float delta_time_mean = 0;
};
