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
}
