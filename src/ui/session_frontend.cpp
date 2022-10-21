#include "session_frontend.h"

#include <imgui.h>

#include "world/world.h"
#include "engine/engine.h"
#include "utils/profiler.h"


SessionFrontend::SessionFrontend()
{
	window_name = "Session Frontend";
	last_frame = Profiler::get().get_last_frame();

	fps_graph.unit = "fps";
	fps_graph.h_lines = {
		ui::GraphData::HLine{"30 fps", 1000 / 30.f, 1, 0, 0},
		ui::GraphData::HLine{"60 fps", 1000 / 60.f, 0.9f, 0.7f, 0.1f},
		ui::GraphData::HLine{"120 fps", 1000 / 120.f, 0, 1, 0}
	};
}

static void compute_record_data(ui::RecordData& output, int level, std::vector<Record>::iterator& it,
                                const std::vector<Record>& records, const TimeType& parent_end, bool main_thread)
{
	if (it == records.end())
		return;

	while (it < records.end() && it->start < parent_end)
	{
		if (main_thread)
		{
			if (it->thread_id != std::this_thread::get_id())
			{
				++it;
				continue;
			}
		}
		else
		{
			if (it->thread_id == std::this_thread::get_id())
			{
				++it;
				continue;
			}
		}

		const float local_min = std::chrono::duration_cast<std::chrono::microseconds>(it->start - records[0].start).
			count() / 1000000.f;
		const float local_max = std::chrono::duration_cast<std::chrono::microseconds>(it->end - records[0].start).
			count() / 1000000.f;

		if (local_min < output.min_value)
			output.min_value = local_min;

		if (local_max > output.max_value)
			output.max_value = local_max;

		TimeType last_start = it->start;
		TimeType last_end = it->end;
		output.get_line(level).emplace_back(ui::RecordData::RecordItem{
			.name = it->name, .start = local_min, .end = local_max
		});
		++it;
		if (it != records.end() && it->start > last_start && it->start < last_end)
		{
			compute_record_data(output, level + 1, it, records, last_end, main_thread);
		}
	}
}

static ui::RecordData compute_record(const std::vector<Record>& records, bool main_thread = true)
{
	ui::RecordData result;

	if (records.empty())
		return result;

	std::vector<Record> records_copy = records;

	auto begin = records_copy.begin();
	if (main_thread)
		while (begin != records_copy.end() && begin->thread_id != std::this_thread::get_id()) ++begin;
	else
		while (begin != records_copy.end() && begin->thread_id == std::this_thread::get_id()) ++begin;

	compute_record_data(result, 0, begin, records_copy, (records_copy.end() - 1)->end, main_thread);
	return result;
}

void SessionFrontend::draw()
{
	STAT_FRAME("Session Frontend Update");

	// Enable or disable profiler
	ImGui::Checkbox("Enable profiler", &Profiler::get().enabled);

	// Profiler need to be enabled to display recorded data
	if (!Profiler::get().enabled)
	{
		record_first_frame = 5;
		return;
	}

	ImGui::Separator();

	if (record_first_frame >= 0)
		record_first_frame--;

	if (!realtime)
		ImGui::SameLine();
	ImGui::Checkbox("realtime", &realtime);
	if (use_custom_width)
		ImGui::DragFloat("scale", &custom_width, 1);
	else
		ImGui::DragFloat("zoom", &zoom);
	ImGui::SameLine();
	if (ImGui::Checkbox("manual scale", &use_custom_width))
		recorded_max = 0;

	if (frame_record.max_value > recorded_max)
		recorded_max = frame_record.max_value;

	if (ImGui::BeginTabBar("SessionFrontendTab"))
	{
		if (ImGui::BeginTabItem("Frame events"))
		{
			ImGui::Separator();
			// Update displayed record
			if (realtime || ImGui::Button("refresh") || record_first_frame == 0)
				frame_record = compute_record(Profiler::get().get_last_frame(), true);

			frame_record.display(zoom, use_custom_width, recorded_max);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Actions"))
		{
			ImGui::Separator();
			// Update displayed record
			if (realtime || ImGui::Button("refresh") || record_first_frame == 0)
			{
				action_record_main_thread = compute_record(Profiler::get().get_actions(), true);
				action_record_other_threads = compute_record(Profiler::get().get_actions(), false);
			}

			float display_max = std::max(action_record_main_thread.max_value, action_record_other_threads.max_value);

			action_record_main_thread.display(zoom, use_custom_width, recorded_max);
			ImGui::Separator();
			action_record_other_threads.display(zoom, use_custom_width, recorded_max);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::Separator();

	// Display framerate
	fps_graph.push_value(static_cast<float>(Engine::get().get_world().get_delta_seconds() * 1000.0));
	fps_graph.display();
}
