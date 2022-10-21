#include "widgets.h"

#include <imgui.h>

namespace ui
{
	bool position_edit(Eigen::Vector3d& position, const std::string& title)
	{
		Eigen::Vector3f pos = position.cast<float>();

		ImGui::Text("%s", title.c_str());
		ImGui::SameLine();
		float width = ImGui::GetContentRegionAvail().x;

		bool edit = false;
		if (ImGui::BeginChild((title + "x").c_str(), ImVec2(width / 3, 20)))
		{
			ImGui::Text("x=");
			ImGui::SameLine();
			if (ImGui::DragFloat("##x", &pos.x())) edit = true;
		}
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::BeginChild((title + "y").c_str(), ImVec2(width / 3, 20)))
		{
			ImGui::Text("y=");
			ImGui::SameLine();
			if (ImGui::DragFloat("##y", &pos.y())) edit = true;
		}
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::BeginChild((title + "z").c_str(), ImVec2(width / 3, 20)))
		{
			ImGui::Text("z=");
			ImGui::SameLine();
			if (ImGui::DragFloat("##z", &pos.z())) edit = true;
		}
		ImGui::EndChild();

		if (edit)
		{
			position = pos.cast<double>();
			return true;
		}
		return false;
	}

	bool rotation_edit(Eigen::Quaterniond& rotation, const std::string& title)
	{
		Eigen::Vector3f eulers = rotation.cast<float>().toRotationMatrix().eulerAngles(0, 1, 2) / M_PI * 180;

		ImGui::Text("%s", title.c_str());
		ImGui::SameLine();
		const float width = ImGui::GetContentRegionAvail().x;

		bool edit = false;
		if (ImGui::BeginChild((title + "x").c_str(), ImVec2(width / 3, 20)))
		{
			ImGui::Text("roll=");
			ImGui::SameLine();
			if (ImGui::DragFloat("##roll", &eulers.x())) edit = true;
		}
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::BeginChild((title + "y").c_str(), ImVec2(width / 3, 20)))
		{
			ImGui::Text("pitch=");
			ImGui::SameLine();
			if (ImGui::DragFloat("##pitch", &eulers.y())) edit = true;
		}
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::BeginChild((title + "z").c_str(), ImVec2(width / 3, 20)))
		{
			ImGui::Text("yaw=");
			ImGui::SameLine();
			if (ImGui::DragFloat("##yaw", &eulers.z())) edit = true;
		}
		ImGui::EndChild();

		if (edit)
		{
			eulers = eulers / 180 * M_PI;
			rotation = Eigen::AngleAxisd(eulers.x(), Eigen::Vector3d::UnitX())
				* Eigen::AngleAxisd(eulers.y(), Eigen::Vector3d::UnitY())
				* Eigen::AngleAxisd(eulers.z(), Eigen::Vector3d::UnitZ());

			return true;
		}
		return false;
	}


	static void draw_record(const ui::RecordData::RecordItem& record, int line_index, float scale, int index,
	                        size_t line_count)
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

	void RecordData::display(float zoom, bool custom_width, float display_max) const
	{
		const float zoom_scale = custom_width ? 1 : std::exp(zoom / 200);
		const float zoom_width = custom_width
			                         ? std::exp(display_max / 200) * 1000
			                         : ImGui::GetContentRegionAvail().x - 20;
		const float box_scales = zoom_width / (custom_width ? display_max : max_value) * zoom_scale;
		const float window_width = zoom_width * zoom_scale;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)));
		if (ImGui::BeginChild(("record" + std::to_string(rand())).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, lines.size() * 25 + static_cast<float>(30)), false,
			ImGuiWindowFlags_HorizontalScrollbar))
		{
			if (ImGui::BeginChild("scrollArea", ImVec2(window_width, lines.size() * static_cast<float>(25)), false,
			                      ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
			{
				int record_index = 0;
				int line_index = 0;
				for (const auto& line : lines)
				{
					for (const auto& record : line)
						draw_record(record, line_index, box_scales, record_index++, lines.size());
					line_index++;
				}
			}
			ImGui::EndChild();
		}
		ImGui::PopStyleColor();
		ImGui::EndChild();
	}

	void GraphData::push_value(float value)
	{
		if (value_vector.empty())
			return;

		value_vector[current_value] = value;
		current_value = (current_value + 1) % value_vector.size();
	}

	void GraphData::display()
	{
		const float height = std::max(150.f, ImGui::GetContentRegionAvail().y);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)));
		if (ImGui::BeginChild("graph", ImVec2(0, height), false))
		{
			const int table_width = static_cast<int>(ImGui::GetContentRegionAvail().x);

			while (value_vector.size() < table_width)
				value_vector.insert(value_vector.begin() + current_value, 0);
			while (value_vector.size() > table_width)
			{
				value_vector.erase(value_vector.begin() + current_value);
				current_value = current_value % value_vector.size();
			}

			float mean = 0;
			int val_count = 0;
			float max = FLT_MIN;
			float min = FLT_MAX;
			for (int i = 0; i < std::min(table_width, mean_update_range); ++i)
			{
				const float val = value_vector[(current_value - i + table_width) % table_width];
				if (val > 0.0001f)
				{
					mean += val;
					val_count++;
				}
				if (val > max)
					max = val;
				if (val < min)
					min = val;
			}
			mean /= static_cast<float>(val_count);

			const float scale = height / (mean * 1.5f);
			const auto draw_start_pos = ImGui::GetWindowPos();
			for (int i = 1; i < table_width; ++i)
			{
				const float a = value_vector[(i - 1 + current_value) % table_width] * scale;
				const float b = value_vector[(i + current_value) % table_width] * scale;

				ImGui::GetWindowDrawList()->AddLine(
					ImVec2(draw_start_pos.x + i, draw_start_pos.y + height - a),
					ImVec2(draw_start_pos.x + i + 1, draw_start_pos.y + height - b),
					ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 0, 1)),
					1
				);
			}

			// Draw horizontal lines
			for (const auto& h_line : h_lines)
			{
				ImGui::GetWindowDrawList()->AddText(
					ImVec2(
						draw_start_pos.x + (h_line.align_left
							                    ? 2
							                    : table_width - ImGui::CalcTextSize(h_line.title.c_str()).x + 5),
						draw_start_pos.y + height - scale * h_line.value + 5),
					ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)),
					h_line.title.c_str());

				ImGui::GetWindowDrawList()->AddLine(
					ImVec2(draw_start_pos.x, draw_start_pos.y + height - scale * h_line.value),
					ImVec2(draw_start_pos.x + table_width, draw_start_pos.y + height - scale * h_line.value),
					ImGui::ColorConvertFloat4ToU32(ImVec4(h_line.r, h_line.g, h_line.b, h_line.a)),
					2
				);
			}

			ImGui::GetWindowDrawList()->AddText(
				ImVec2(draw_start_pos.x + table_width - 50, draw_start_pos.y + height - scale * mean + 5),
				ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)),
				(std::to_string(static_cast<int>(1 / mean * 1000)) + " " + unit).c_str());
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
}
