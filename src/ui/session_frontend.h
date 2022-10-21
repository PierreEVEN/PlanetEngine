#pragma once
#include <vector>

#include "ui.h"
#include "widgets.h"
#include "utils/profiler.h"

struct Record;

class SessionFrontend final : public ImGuiWindow
{
public:
	SessionFrontend();
	~SessionFrontend() override = default;

	void draw() override;
private:
	std::vector<Record> last_frame;
	ui::RecordData frame_record;
	ui::RecordData action_record_main_thread;
	ui::RecordData action_record_other_threads;
	float zoom = 0;
	bool realtime = false;
	float custom_width = 0;
	bool use_custom_width = false;
	float recorded_max = 0;

	int record_first_frame = 5;
	
	ui::GraphData fps_graph;
};
