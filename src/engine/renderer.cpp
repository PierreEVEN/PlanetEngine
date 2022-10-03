#include "renderer.h"

#include <fbo.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <texture2d.h>
#include <vao.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "engine.h"
#include "graphics/material.h"
#include "graphics/texture_image.h"

static bool initialized_opengl = false;

static Eigen::Vector2i default_window_res = {1920, 1080};

static void glfw_error_callback(int error, const char* description)
{
	std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

Renderer::Renderer()
{
	if (!initialized_opengl)
	{
		initialized_opengl = true;
		init_context();
	}

	std::vector<std::shared_ptr<EZCOGL::Texture2D>> textures;
	// GBuffer color
	g_buffer_color = TextureImage::create("gbuffer-color");
	g_buffer_color->alloc(default_window_res.x(), default_window_res.y(), GL_RGB16F, nullptr);
	textures.push_back(g_buffer_color);

	// GBuffer positions
	g_buffer_position = TextureImage::create("gbuffer-position");
	g_buffer_position->alloc(default_window_res.x(), default_window_res.y(), GL_RGB32F, nullptr);
	textures.push_back(g_buffer_position);

	// GBuffer normal
	g_buffer_normal = TextureImage::create("gbuffer-normal");
	g_buffer_normal->alloc(default_window_res.x(), default_window_res.y(), GL_RGB16F, nullptr);
	textures.push_back(g_buffer_normal);

	// GBuffer depth
	g_buffer_depth = TextureImage::create("gbuffer-depths");
	g_buffer_depth->alloc(default_window_res.x(), default_window_res.y(), GL_DEPTH_COMPONENT32F, nullptr);
	g_buffer = EZCOGL::FBO_DepthTexture::create(textures, g_buffer_depth);


	// Resolve buffer
	resolve_texture = TextureImage::create("resolve");
	resolve_texture->alloc(default_window_res.x(), default_window_res.y(), GL_RGB8, nullptr);
	textures.push_back(g_buffer_normal);
	resolve_framebuffer = EZCOGL::FBO::create({resolve_texture});

	Engine::get().on_framebuffer_resized.add_object(this, &Renderer::resize_framebuffer_internal);
}

Renderer::~Renderer()
{
	Engine::get().on_framebuffer_resized.clear_object(this);
	glfwDestroyWindow(main_window);
}

void Renderer::initialize() const
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	glfwMakeContextCurrent(main_window);

	glfwPollEvents();
	EZCOGL::VAO::none()->bind();

	int w, h;
	glfwGetWindowSize(main_window, &w, &h);
	ImGui::SetNextWindowPos(ImVec2(fullscreen ? -100.f : -4.f, 18));
	ImGui::SetNextWindowSize(fullscreen ? ImVec2(1, 1) : ImVec2(static_cast<float>(w) + 8, static_cast<float>(h) - 14));
	if (ImGui::Begin("Master Window", nullptr,
	                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
	                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground))
	{
		ImGui::DockSpace(ImGui::GetID("Master dockSpace"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_PassthruCentralNode);
	}
	ImGui::End();
}

void Renderer::bind_g_buffers() const
{
	g_buffer->bind();
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glFrontFace(GL_CCW);
	glClearColor(0, 0, 0, 0);
	glClearDepth(0.0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	EZCOGL::VAO::none()->bind();
}

void Renderer::bind_deferred_combine() const
{
	if (fullscreen)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	else
		resolve_framebuffer->bind();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glFrontFace(GL_CCW);
	glClearColor(1, 0, 1, 0);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	EZCOGL::VAO::none()->bind();
}

void Renderer::submit() const
{
	if (!fullscreen)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glFrontFace(GL_CCW);
		glClearColor(0, 0, 0, 0);
		glClearDepth(1.0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(main_window);
}

bool Renderer::should_close() const
{
	return glfwWindowShouldClose(main_window);
}

void Renderer::switch_fullscreen()
{
	Engine::get().on_framebuffer_resized.clear_object(this);
	fullscreen = !fullscreen;
	if (fullscreen)
	{
		int w, h;
		glfwGetWindowSize(main_window, &w, &h);
		Engine::get().on_framebuffer_resized.add_object(this, &Renderer::resize_framebuffer_internal);
		resize_framebuffer_internal(nullptr, w, h);
	}
	on_fullscreen.execute(fullscreen);
}

void Renderer::resize_framebuffer_internal(GLFWwindow*, int x, int y)
{
	framebuffer().resize(x, y);
	resolve_framebuffer->resize(x, y);

	Engine::get().get_world().get_camera()->viewport_res() = { x, y };
}

void Renderer::init_context()
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) { std::cerr << "Failed to initialize GFLW!" << std::endl; }

	const char* glsl_version = "#version 330";
	GLint major = 4, minor = 5;
#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
#endif

	main_window = glfwCreateWindow(default_window_res.x(), default_window_res.y(), "Project", nullptr, nullptr);
	if (!main_window) { std::cerr << "Failed to create Window!" << std::endl; }

	glfwMakeContextCurrent(main_window);

	bool err = gl3wInit() != 0;
	if (err) { std::cerr << "Failed to initialize OpenGL loader!" << std::endl; }

	glfwSwapInterval(1); // Enable vsync
	glfwSetWindowUserPointer(main_window, this);

	IMGUI_CHECKVERSION();
	imgui_context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.Fonts->AddFontFromFileTTF("resources/fonts/Roboto-Medium.ttf", 16.f);
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;

	ImGui::GetStyle().Colors[ImGuiCol_FrameBg, ImGui::ColorConvertFloat4ToU32(ImVec4(46, 46, 46, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_TitleBg, ImGui::ColorConvertFloat4ToU32(ImVec4(30, 30, 30, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_CheckMark, ImGui::ColorConvertFloat4ToU32(ImVec4(159, 159, 159, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_SliderGrab, ImGui::ColorConvertFloat4ToU32(ImVec4(159, 159, 159, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_Button, ImGui::ColorConvertFloat4ToU32(ImVec4(69, 69, 69, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_Header, ImGui::ColorConvertFloat4ToU32(ImVec4(71, 71, 71, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_Tab, ImGui::ColorConvertFloat4ToU32(ImVec4(21, 21, 21, 220))];
	ImGui::GetStyle().Colors[ImGuiCol_TabUnfocused, ImGui::ColorConvertFloat4ToU32(ImVec4(27, 27, 27, 220))];
	ImGui::GetStyle().Colors[ImGuiCol_TabUnfocusedActive, ImGui::ColorConvertFloat4ToU32(ImVec4(52, 52, 52, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg, ImGui::ColorConvertFloat4ToU32(ImVec4(18, 18, 18, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive, ImGui::ColorConvertFloat4ToU32(ImVec4(29, 29, 29, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_WindowBg, ImGui::ColorConvertFloat4ToU32(ImVec4(15, 15, 15, 255))];
	ImGui::GetStyle().Colors[ImGuiCol_TabActive, ImGui::ColorConvertFloat4ToU32(ImVec4(135, 135, 135, 255))];

	auto& style = ImGui::GetStyle();
	style.WindowRounding = 0;
	style.ScrollbarRounding = 0;
	style.TabRounding = 0;
	style.WindowBorderSize = 1;
	style.PopupBorderSize = 1;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5);
	style.WindowPadding = ImVec2(4, 4);
	style.GrabMinSize = 12;
	style.ScrollbarSize = 16;
	style.IndentSpacing = 17;
	/*
	style.FramePadding = ImVec2(12, 10);
	*/

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

	if (major > 4 || major == 4 && minor >= 5 ||
		glfwExtensionSupported("GL_ARB_clip_control"))
	{
		glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	}
	else
	{
		std::cerr << "glClipControl is required but not supported" << std::endl;
		exit(EXIT_FAILURE);
	}
}
