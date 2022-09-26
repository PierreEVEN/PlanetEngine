#pragma once

#include <gl_eigen.h>


struct GLFWwindow;
struct ImGuiContext;

class Renderer final
{
public:
	Renderer();
	~Renderer();

	void begin() const;
	void end() const;

	[[nodiscard]] bool should_close() const;
	[[nodiscard]] GLFWwindow* get_window() const { return main_window; }

private:
	void init_context();

	GLFWwindow* main_window = nullptr;
	ImGuiContext* imgui_context = nullptr;
	bool uniform_explicit_location_support;
};
