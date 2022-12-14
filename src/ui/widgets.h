#pragma once

#include <Eigen/Dense>

#ifndef FLT_MIN
#define FLT_MIN 1.175494351e-38F
#endif // FLT_MIN

namespace ui
{
	bool position_edit(Eigen::Vector3d& position, const std::string& title);
	bool rotation_edit(Eigen::Quaterniond& position, const std::string& title);

	struct RecordData
	{
		struct RecordItem
		{
			std::string name;
			float start;
			float end;
		};

		std::vector<std::vector<RecordItem>> lines;
		float min_value = FLT_MIN;
		float max_value = FLT_MIN;

		float min_display_value = 0;
		float max_display_value = 1;

		std::string label;

		std::vector<RecordItem>& get_line(int index)
		{
			if (index >= static_cast<int>(lines.size()))
				lines.resize(index + 1);
			return lines[index];
		}
		// Return true when display min and max have been changed
		bool display();
	private:
		float drag_start_x = 0;
		bool pressed = false;
	};



	class GraphData
	{
	public:
		struct HLine
		{
			std::string title;
			float value;
			float r = 1, g = 1, b = 1, a = 1;
			bool align_left = true;
		};

		void display();
		void push_value(float value);
		std::string unit;
		int mean_update_range = 30;
		std::vector<HLine> h_lines;
	private:
		std::vector<float> value_vector;
		int current_value = 0;
	};
}
