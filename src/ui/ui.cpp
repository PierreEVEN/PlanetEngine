#include <fbo.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>
#include <ui/ui.h>

#include "camera.h"
#include "world.h"

static bool show_debug_window = false;
static bool enable_vsync = true;
static bool show_demo_window = false;

namespace ui
{
	void draw(const Renderer& renderer, const World& world, const std::shared_ptr<EZCOGL::FBO_DepthTexture>& g_buffer)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					exit(0);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Debug"))
			{
				ImGui::Checkbox("framebuffers", &show_debug_window);
				if (ImGui::Checkbox("Enable VSync", &enable_vsync))
					glfwSwapInterval(enable_vsync ? 1 : 0);
				ImGui::Checkbox("demo window", &show_demo_window);
				ImGui::EndMenu();
			}

			ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 100, 0));
			ImGui::Text("%f fps", 1.0f / world.get_delta_seconds());

			ImGui::EndMainMenuBar();
		}

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		if (show_debug_window)
		{
			const float available_width = ImGui::GetContentRegionAvail().x;
			if (ImGui::Begin("framebuffers", &show_debug_window))
			{
				for (int i = 0; i < g_buffer->nb_textures(); ++i)
				{
					const float ratio = available_width / g_buffer->width();
					ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(g_buffer->texture(i)->id())),
					             ImVec2(g_buffer->width() * ratio, g_buffer->height() * ratio), ImVec2(0, 1),
					             ImVec2(1, 0));
				}
				if (g_buffer->depth_texture())
				{
					const float ratio = available_width / g_buffer->width();
					ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(g_buffer->depth_texture()->id())),
					             ImVec2(g_buffer->width() * ratio, g_buffer->height() * ratio), ImVec2(0, 1),
					             ImVec2(1, 0));
				}
			}
			ImGui::End();
		}
	}


	void EditTransform(Camera& camera, Eigen::Matrix4f& matrix)
	{
		static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
		static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
		if (ImGui::IsKeyPressed(90))
			mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed(69))
			mCurrentGizmoOperation = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed(82)) // r Key
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
		if (ImGui::IsKeyPressed(83))
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
		ImGuizmo::Manipulate(Eigen::Matrix4f(camera.view_matrix().cast<float>()).data(), Eigen::Matrix4f(camera.projection_matrix().cast<float>()).data(), mCurrentGizmoOperation, mCurrentGizmoMode, matrix.data(), NULL, useSnap ? &snap.x() : NULL);
	}


}
