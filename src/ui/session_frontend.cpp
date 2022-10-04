#include "session_frontend.h"

#include <imgui.h>
#include <iostream>

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
	const char* name;
	float start;
	float end;
};

void SessionFrontend::draw()
{
	ImGui::Checkbox("record stats", &Profiler::get().enabled);
	if (ImGui::Button("record")) {
		last_frame = Profiler::get().get_last_frame();
		data = {};
		auto begin = last_frame.begin();
		compute_record_data(data, 0, begin, last_frame);
	}
	ImGui::Separator();

	float width = 1 / 30 * 2000;


	if (ImGui::BeginChild("times", ImVec2(width, 0)))
	{
		if (last_frame.size() > 0)
		{
			draw_record_data(data);
		}
	}
	ImGui::EndChild();
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
		output.get_line(level).emplace_back(DrawRecord{ .name = it->name, .start = local_min, .end = local_max });
		++it;
		if (it != records.end() && it->start > last_start) // && it->start < last_end)
		{
			compute_record_data(output, level + 1, it, records);
		}
	}
}

void SessionFrontend::draw_record_data(const RecordData& data) const
{
	const float zoom_width = ImGui::GetContentRegionAvail().x - 20;
	const float scale = zoom_width / data.max_value;
	if (ImGui::BeginChild("record", ImVec2(zoom_width, 300), true))
	{
		float last_end = 0;
		int index = 0;
		for (const auto& line : data.lines)
		{
			ImGui::Text("line %d : ", index++);
			for (const auto& record : line)
			{
				ImGui::SameLine();
				ImGui::Dummy(ImVec2((record.start - last_end) * scale - 2, 0));
				ImGui::SameLine();
				ImGui::Button(
					(record.name + std::string(" : ") + std::to_string((record.end - record.start) * 1000) + "ms").
					c_str(), ImVec2((record.end - record.start) * scale + 2, 20));
				last_end = record.start;
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text("%s : %f ms / start : %f / end : %f", record.name, (record.end - record.start) * 1000,
						record.start * 1000, record.end * 1000);
					ImGui::EndTooltip();
				}
			}
		}
	}
	ImGui::EndChild();
}
