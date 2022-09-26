#include "renderer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <vao.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>


static void glfw_error_callback(int error, const char* description)
{
	std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

Renderer::Renderer()
{
	init_context();
}

Renderer::~Renderer()
{
	glfwDestroyWindow(main_window);
}

void Renderer::begin() const
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	glfwMakeContextCurrent(main_window);

	glfwPollEvents();
}

void Renderer::end() const
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(main_window);
}

bool Renderer::should_close() const
{
	return glfwWindowShouldClose(main_window);
}

void Renderer::init_context()
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) { std::cerr << "Failed to initialize GFLW!" << std::endl; }

	const char* glsl_version = "#version 330";
#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#endif

	main_window = glfwCreateWindow(800, 600, "Project", nullptr, nullptr);
	if (!main_window) { std::cerr << "Failed to create Window!" << std::endl; }

	glfwMakeContextCurrent(main_window);

	bool err = gl3wInit() != 0;
	if (err) { std::cerr << "Failed to initialize OpenGL loader!" << std::endl; }

	glfwSwapInterval(1); // Enable vsync
	glfwSetWindowUserPointer(main_window, this);

	IMGUI_CHECKVERSION();
	imgui_context = ImGui::CreateContext();
	const ImGuiIO& io = ImGui::GetIO();
	(void)io;

#ifdef _WIN32
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	float xscale, yscale;
	glfwGetMonitorContentScale(monitor, &xscale, &yscale);
	if (xscale > 1 || yscale > 1)
	{
		const float highDPIscaleFactor = xscale;
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(highDPIscaleFactor);
		ImGui::GetIO().FontGlobalScale = highDPIscaleFactor;
	}
#endif

	ImGui::StyleColorsDark(); //ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(main_window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	std::cout << glGetString(GL_VENDOR) << std::endl;
	std::cout << glGetString(GL_RENDERER) << std::endl;
	std::cout << glGetString(GL_VERSION) << std::endl;

	int nb_ext;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nb_ext);
	uniform_explicit_location_support = false;
	for (int i = 0; i < nb_ext; ++i)
	{
		const char* name = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
		if (std::string(name) == "GL_ARB_explicit_uniform_location")
			uniform_explicit_location_support = true;
	}

	//necessary for non mutiple of 4 width image loading into texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
}