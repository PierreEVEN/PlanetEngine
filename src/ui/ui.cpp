#include <fbo.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <ui/ui.h>

#include "asset_manager_ui.h"
#include "camera.h"
#include "graphic_debugger.h"
#include "world.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "utils/profiler.h"

static bool show_demo_window = false;

class ImGuiDemoWindow : public ImGuiWindow
{
public:
	ImGuiDemoWindow()
	{
		window_name = "demo window";
	}
	void draw() override
	{
		ImGui::ShowDemoWindow(&open);
		
		if (!open)
			close();
	}

	bool open = true;
};

namespace ui
{
	void draw()
	{
		STAT_DURATION(ImGui_UI);
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					exit(EXIT_SUCCESS);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::MenuItem("Texture view")) ImGuiWindow::create_window<TextureManagerUi>();
				if (ImGui::MenuItem("Material view")) ImGuiWindow::create_window<MaterialManagerUi>();
				if (ImGui::MenuItem("Mesh view")) ImGuiWindow::create_window<MeshManagerUi>();
				ImGui::Separator();
				if (ImGui::MenuItem("Graphic debugger")) ImGuiWindow::create_window<GraphicDebugger>();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Debug"))
			{
				if (ImGui::MenuItem("Demo window")) ImGuiWindow::create_window<ImGuiDemoWindow>();
				ImGui::EndMenu();
			}

			ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 100, 0));
			ImGui::Text("%f fps", 1.0 / Engine::get().get_world().get_delta_seconds());

			ImGui::EndMainMenuBar();
		}

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		ImGuiWindow::draw_all();
	}


	void EditTransform(Camera& camera, Eigen::Matrix4f& matrix)
	{
		static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
		static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
		if (ImGui::IsKeyPressed(ImGuiKey_T))
			mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed(ImGuiKey_R))
			mCurrentGizmoOperation = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed(ImGuiKey_S)) // r Key
			mCurrentGizmoOperation = ImGuizmo::SCALE;
		if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
			mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
			mCurrentGizmoOperation = ImGuizmo::ROTATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
			mCurrentGizmoOperation = ImGuizmo::SCALE;
		float matrixTranslation[3], matrixRotation[3], matrixScale[3];
		ImGuizmo::DecomposeMatrixToComponents(matrix.data(), matrixTranslation, matrixRotation, matrixScale);
		ImGui::InputFloat3("Tr", matrixTranslation);
		ImGui::InputFloat3("Rt", matrixRotation);
		ImGui::InputFloat3("Sc", matrixScale);
		ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix.data());

		if (mCurrentGizmoOperation != ImGuizmo::SCALE)
		{
			if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
				mCurrentGizmoMode = ImGuizmo::LOCAL;
			ImGui::SameLine();
			if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
				mCurrentGizmoMode = ImGuizmo::WORLD;
		}
		static bool useSnap(false);
		if (ImGui::IsKeyPressed(ImGuiKey_W))
			useSnap = !useSnap;
		ImGui::Checkbox("use snap", &useSnap);
		ImGui::SameLine();
		Eigen::Vector3f snap;
		switch (mCurrentGizmoOperation)
		{
		case ImGuizmo::TRANSLATE:
			snap = Eigen::Vector3f(1, 1, 1);
			ImGui::InputFloat3("Snap", &snap.x());
			break;
		case ImGuizmo::ROTATE:
			snap = Eigen::Vector3f(1, 1, 1);
			ImGui::InputFloat("Angle Snap", &snap.x());
			break;
		case ImGuizmo::SCALE:
			snap = Eigen::Vector3f(1, 1, 1);
			ImGui::InputFloat("Scale Snap", &snap.x());
			break;
		}

		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
		ImGuizmo::Manipulate(Eigen::Matrix4f(camera.view_matrix().cast<float>()).data(),
		                     Eigen::Matrix4f(camera.reversed_z_projection_matrix().cast<float>()).data(),
		                     mCurrentGizmoOperation, mCurrentGizmoMode, matrix.data(), NULL,
		                     useSnap ? &snap.x() : NULL);
	}
}

static std::vector<std::shared_ptr<ImGuiWindow>> imgui_windows;
static size_t current_window_id = 0;
size_t ImGuiWindow::make_window_id()
{
	return current_window_id++;
}

void ImGuiWindow::register_window_internal(std::shared_ptr<ImGuiWindow> window)
{
	imgui_windows.emplace_back(window);
}

void ImGuiWindow::draw_all()
{
	for (int64_t i = static_cast<int64_t>(imgui_windows.size()) - 1; i >= 0; --i)
		if (!imgui_windows[i]->is_open)
			imgui_windows.erase(imgui_windows.begin() + i);

	for (const auto& window : imgui_windows)
		window->draw_internal();
}

void ImGuiWindow::draw_internal()
{
	if (Engine::get().get_renderer().is_fullscreen())
		return;

	if (ImGui::Begin((window_name + "##" + std::to_string(window_id)).c_str(), &is_open))
		draw();
	ImGui::End();
}
