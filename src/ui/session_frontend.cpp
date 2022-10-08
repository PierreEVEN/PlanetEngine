#include "session_frontend.h"

#include <imgui.h>
#include <cmath>

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
	if (record_first_frame > 0) {
		record_first_frame--;
		if (record_first_frame == 0) {
			data = {};
			auto begin = last_frame.begin();
			compute_record_data(data, 0, begin, last_frame);
		}
	}


	ImGui::Checkbox("Enable profiler", &Profiler::get().enabled);
	if (!Profiler::get().enabled)
		return;

	ImGui::Separator();

	if (realtime || ImGui::Button("refresh"))
	{
		last_frame = Profiler::get().get_last_frame();
		data = {};
		auto begin = last_frame.begin();
		compute_record_data(data, 0, begin, last_frame);
	}
	if (!realtime)
		ImGui::SameLine();
	ImGui::Checkbox("realtime", &realtime);

	ImGui::Separator();
	if (last_frame.size() > 0)
	{
		draw_record_data(data);
	}
	ImGui::Separator();

	if (use_custom_width)
		ImGui::DragFloat("scale", &custom_width, 1);
	else
		ImGui::DragFloat("zoom", &zoom);
	ImGui::SameLine();
	if (ImGui::Checkbox("manual scale", &use_custom_width))
		recorded_max = 0;

	if (data.max_value > recorded_max)
		recorded_max = data.max_value;
}

std::vector<DrawRecord>& SessionFrontend::RecordData::get_line(int index)
{
	if (index >= static_cast<int>(lines.size()))
		lines.resize(index + 1);
	return lines[index];
}

void SessionFrontend::compute_record_data(RecordData& output, int level, std::vector<Record>::iterator& it,
                                          const std::vector<Record>& records)
{
	while (it < records.end())
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
		if (it != records.end() && (it->start > last_start && it->start < last_end))
		{
			compute_record_data(output, level + 1, it, records);
		}
	}
}
void SessionFrontend::draw_record(const DrawRecord& record, int line_index, float scale, int index) const {
	const auto draw_start_pos = ImGui::GetWindowPos();
	
	ImGui::BeginGroup();

	ImVec2 min = ImVec2(draw_start_pos.x + record.start * scale, draw_start_pos.y + line_index * 25.f);
	ImVec2 max = ImVec2(draw_start_pos.x + record.end * scale + 2.f, draw_start_pos.y + line_index * 25.f + 24.f);

	float r, g, b;

	ImGui::ColorConvertHSVtoRGB(std::fmod(static_cast<float>(index) / 3.533f, 1 - line_index / 10.0f), 0.6f, 1, r, g, b);

	ImGui::PushClipRect(min, max, true);
	ImGui::GetWindowDrawList()->AddRectFilled(
		min, max,
		ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1)));
	ImGui::GetWindowDrawList()->AddText(ImVec2(draw_start_pos.x + record.start * scale, draw_start_pos.y + line_index * 25.f), ImGui::ColorConvertFloat4ToU32(ImVec4(0.86f, 0.96f, 0.96f, 1)), record.name.c_str());
	ImGui::PopClipRect();
	ImGui::EndGroup();

	if (ImGui::IsMouseHoveringRect(min, max))
	{
		ImGui::BeginTooltip();
		ImGui::Text("%s : %f ms / start : %f / end : %f", record.name.c_str(), (record.end - record.start) * 1000,
			record.start * 1000, record.end * 1000);
		ImGui::EndTooltip();
	}
}

void SessionFrontend::draw_record_data(const RecordData& data) const
{
	const float zoom_scale = use_custom_width ? 1 : std::exp(zoom / 200);
	const float zoom_width = use_custom_width ? std::exp(custom_width / 200) * 1000 : ImGui::GetContentRegionAvail().x - 20;
	const float box_scales = zoom_width / (use_custom_width ? recorded_max : data.max_value) * zoom_scale;
	const float window_width = zoom_width * zoom_scale;
	int record_index = 0;
	if (ImGui::BeginChild("record", ImVec2(ImGui::GetContentRegionAvail().x, data.lines.size() * 25 + static_cast<float>(25)), false, ImGuiWindowFlags_HorizontalScrollbar))
	{
		if (ImGui::BeginChild("scrollArea", ImVec2(window_width, data.lines.size() * static_cast<float>(25)), false)) {
			int line_index = 0;
			for (const auto& line : data.lines)
			{
				for (const auto& record : line)
				{
					draw_record(record, line_index, box_scales, record_index++);
				}
				line_index++;
			}
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}
