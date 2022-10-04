#include "session_frontend.h"

#include <imgui.h>

#include "utils/profiler.h"

SessionFrontend::SessionFrontend()
{
	window_name = "Session Frontend";
	last_frame = Profiler::get().get_last_frame();
}

SessionFrontend::~SessionFrontend()
{
}

static void draw_child(std::vector<Record>::iterator current, const std::vector<Record>::iterator& end, int level)
{
	auto last_end = current->end;

	for (; current != end; ++current)
	{
		const Record& current_value = *current;




		//@TODO


		last_end = current_value.end;
	}
}


void SessionFrontend::draw()
{
	ImGui::Checkbox("record stats", &Profiler::get().enabled);
	if (ImGui::Button("record"))
		last_frame = Profiler::get().get_last_frame();
	ImGui::Separator();

	float width = 1 / 30 * 2000;


	if (ImGui::BeginChild("times", ImVec2(width, 0)))
	{
		//draw_child(last_frame.begin(), last_frame.end(), 0);

	}
	ImGui::EndChild();
}
