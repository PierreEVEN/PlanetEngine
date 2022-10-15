#include "session_frontend.h"

#include <imgui.h>
#include <cmath>

#include "world/world.h"
#include "engine/engine.h"
#include "utils/profiler.h"

SessionFrontend::SessionFrontend()
{
	window_name = "Session Frontend";
	last_frame = Profiler::get().get_last_frame();
}

SessionFrontend::~SessionFrontend()
{
}

struct DrawRecord
{
	std::string name;
	float start;
	float end;
};

void SessionFrontend::draw()
{
	STAT_DURATION("Session Frontend Update");
	ImGui::Checkbox("Enable profiler", &Profiler::get().enabled);
	if (!Profiler::get().enabled) {
		record_first_frame = 5;
		return;
	}

	if (record_first_frame >= 0) {
		record_first_frame--;
	}

	ImGui::Separator();

	if (realtime || ImGui::Button("refresh") || record_first_frame == 0)
	{
		last_frame = Profiler::get().get_last_frame();
		data = {};
		auto begin = last_frame.begin();
		if (begin != last_frame.end())
			compute_record_data(data, 0, begin, last_frame, (last_frame.end() - 1)->end);
	}
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

	if (data.max_value > recorded_max)
		recorded_max = data.max_value;

	ImGui::Separator();
	if (last_frame.size() > 0)
	{
		draw_record_data(data);
	}
	ImGui::Separator();
	
	update_draw_delta_seconds();
}

std::vector<DrawRecord>& SessionFrontend::RecordData::get_line(int index)
{
	if (index >= static_cast<int>(lines.size()))
		lines.resize(index + 1);
	return lines[index];
}

void SessionFrontend::compute_record_data(RecordData& output, int level, std::vector<Record>::iterator& it,
                                          const std::vector<Record>& records, const TimeType& parent_end)
{
	if (it == records.end())
		return;

	while (it < records.end() && it->start < parent_end)
	{
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
		output.get_line(level).emplace_back(DrawRecord{.name = it->name, .start = local_min, .end = local_max});
		++it;
		if (it != records.end() && it->start > last_start && it->start < last_end)
		{
			compute_record_data(output, level + 1, it, records, last_end);
		}
	}
}

void SessionFrontend::draw_record(const DrawRecord& record, int line_index, float scale, int index,
                                  size_t line_count) const
{
	const auto draw_start_pos = ImGui::GetWindowPos();

	ImGui::BeginGroup();

	ImVec2 min = ImVec2(draw_start_pos.x + record.start * scale, draw_start_pos.y + line_index * 25.f);
	ImVec2 max = ImVec2(draw_start_pos.x + record.end * scale + 2.f, draw_start_pos.y + line_index * 25.f + 24.f);

	float r, g, b;

	float value = 0.85f;
	if (ImGui::IsMouseHoveringRect(min, max))
	{
		value = 1;
	}
	ImGui::ColorConvertHSVtoRGB(static_cast<float>(index) / 64.422f,
	                            1 - static_cast<float>(line_index) / static_cast<float>(line_count), value, r, g,
	                            b);

	ImGui::PushClipRect(min, max, true);
	ImGui::GetWindowDrawList()->AddRectFilled(
		min, max,
		ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1)));
	ImGui::GetWindowDrawList()->AddText(
		ImVec2(draw_start_pos.x + record.start * scale + 2, draw_start_pos.y + line_index * 25.f),
		ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), record.name.c_str());
	ImGui::PopClipRect();
	ImGui::EndGroup();

	if (ImGui::IsMouseHoveringRect(min, max))
	{
		ImGui::BeginTooltip();
		ImGui::Text("[%s] :", record.name.c_str());
		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::Text("%f ms", (record.end - record.start) * 1000);
		ImGui::Text("From : %f ms", record.start * 1000);
		ImGui::Text("to : %f ms", record.end * 1000);
		ImGui::EndGroup();
		ImGui::EndTooltip();
	}
}

void SessionFrontend::update_draw_delta_seconds()
{
	float height = std::max(150.f, ImGui::GetContentRegionAvail().y);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertFloat4ToU32(ImVec4(63 / 256.f, 72 / 256.f, 69 / 256.f, 1)));
	if (ImGui::BeginChild("framerate", ImVec2(0, height), false))
	{
		const int table_width = static_cast<int>(ImGui::GetContentRegionAvail().x);

		while (delta_time_history.size() < table_width)
			delta_time_history.insert(delta_time_history.begin() + delta_time_index, 0);
		while (delta_time_history.size() > table_width)
		{
			delta_time_history.erase(delta_time_history.begin() + (delta_time_index));
			delta_time_index = delta_time_index % delta_time_history.size();
		}

		delta_time_history[delta_time_index] = static_cast<float>(Engine::get().get_world().get_delta_seconds() *
			1000.0);
		delta_time_index = (delta_time_index + 1) % table_width;

		float mean = 0;
		int val_count = 0;
		float max = 0;
		float min = FLT_MAX;
		for (int i = 0; i < std::min(table_width, 30); ++i)
		{
			const float val = delta_time_history[(delta_time_index - i + table_width) % table_width];
			if (val > 0.0001)
			{
				mean += val;
				val_count++;
			}
			if (val > max)
				max = val;
			if (val < min)
				min = val;
		}
		mean /= val_count;

		const float scale = height / (mean * 1.5f);
		const auto draw_start_pos = ImGui::GetWindowPos();
		for (int i = 1; i < table_width; ++i)
		{

			float a = delta_time_history[(i - 1 + delta_time_index) % table_width] * scale;
			float b = delta_time_history[(i + delta_time_index) % table_width] * scale;


			ImGui::GetWindowDrawList()->AddLine(
				ImVec2(draw_start_pos.x + i, draw_start_pos.y + height - a),
				ImVec2(draw_start_pos.x + i + 1, draw_start_pos.y + height - b),
				ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 0, 1)),
				1
			);
		}

		ImGui::GetWindowDrawList()->AddText(
			ImVec2(draw_start_pos.x + 2, draw_start_pos.y + +height - scale * 1000 / 30 + 5),
			ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), "30 fps");
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(draw_start_pos.x, draw_start_pos.y + height - scale * 1000 / 30),
			ImVec2(draw_start_pos.x + table_width, draw_start_pos.y + height - scale * 1000 / 30),
			ImGui::ColorConvertFloat4ToU32(ImVec4(1, 0, 0, 1)),
			2
		);

		ImGui::GetWindowDrawList()->AddText(
			ImVec2(draw_start_pos.x + 2, draw_start_pos.y + +height - scale * 1000 / 60 + 5),
			ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), "60 fps");
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(draw_start_pos.x, draw_start_pos.y + height - scale * 1000 / 60),
			ImVec2(draw_start_pos.x + table_width, draw_start_pos.y + height - scale * 1000 / 60),
			ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.7f, 0.1f, 1)),
			2
		);

		ImGui::GetWindowDrawList()->AddText(
			ImVec2(draw_start_pos.x + 2, draw_start_pos.y + +height - scale * 1000 / 120 + 5),
			ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), "120 fps");
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(draw_start_pos.x, draw_start_pos.y + height - scale * 1000 / 120),
			ImVec2(draw_start_pos.x + table_width, draw_start_pos.y + height - scale * 1000 / 120),
			ImGui::ColorConvertFloat4ToU32(ImVec4(0, 1, 0, 1)),
			2
		);

		ImGui::GetWindowDrawList()->AddText(
			ImVec2(draw_start_pos.x + table_width - 50, draw_start_pos.y + +height - scale * mean + 5),
			ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), (std::to_string(static_cast<int>(1 / mean * 1000)) + " fps").c_str());
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(draw_start_pos.x, draw_start_pos.y + height - scale * mean),
			ImVec2(draw_start_pos.x + table_width, draw_start_pos.y + height - scale * mean),
			ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)),
			2
		);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void SessionFrontend::draw_record_data(const RecordData& data) const
{
	const float zoom_scale = use_custom_width ? 1 : std::exp(zoom / 200);
	const float zoom_width = use_custom_width
		                         ? std::exp(custom_width / 200) * 1000
		                         : ImGui::GetContentRegionAvail().x - 20;
	const float box_scales = zoom_width / (use_custom_width ? recorded_max : data.max_value) * zoom_scale;
	const float window_width = zoom_width * zoom_scale;
	int record_index = 0;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)));
	if (ImGui::BeginChild(
		"record", ImVec2(ImGui::GetContentRegionAvail().x, data.lines.size() * 25 + static_cast<float>(30)), false,
		ImGuiWindowFlags_HorizontalScrollbar))
	{
		if (ImGui::BeginChild("scrollArea", ImVec2(window_width, data.lines.size() * static_cast<float>(25)), false,
		                      ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
		{
			int line_index = 0;
			for (const auto& line : data.lines)
			{
				for (const auto& record : line)
				{
					draw_record(record, line_index, box_scales, record_index++, data.lines.size());
				}
				line_index++;
			}
		}
		ImGui::EndChild();
	}
	ImGui::PopStyleColor();
	ImGui::EndChild();
}
