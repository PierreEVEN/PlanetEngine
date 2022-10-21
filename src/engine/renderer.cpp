#include "renderer.h"

#include <fbo.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <vao.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "graphics/camera.h"
#include "engine.h"
#include "graphics/easycppogl_texture.h"
#include "graphics/material.h"
#include "graphics/texture_image.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"

#define COLOR_DARKNESS 400.f
#define COLOR(r, g, b, a) ImVec4(r / COLOR_DARKNESS, g / COLOR_DARKNESS, b / COLOR_DARKNESS, a / 255.f)
#define COLOR_BR(r, g, b, a) ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f)

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
	GL_CHECK_ERROR();

	std::vector<std::shared_ptr<EZCOGL::TextureInterface>> textures;
	// GBuffer color
	g_buffer_color = EasyCppOglTexture::create("gbuffer-color", {.filtering_min = TextureMinFilter::Nearest });
	g_buffer_color->set_data_interface(default_window_res.x(), default_window_res.y(), GL_RGB16F);
	textures.push_back(g_buffer_color);

	// GBuffer normal
	g_buffer_normal = EasyCppOglTexture::create("gbuffer-normal", {.filtering_min = TextureMinFilter::Nearest });
	g_buffer_normal->set_data_interface(default_window_res.x(), default_window_res.y(), GL_RGB16F);
	textures.push_back(g_buffer_normal);

	// GBuffer depth
	g_buffer_depth = EasyCppOglTexture::create("gbuffer-depths", {.filtering_min = TextureMinFilter::Nearest});
	g_buffer_depth->set_data_interface(default_window_res.x(), default_window_res.y(), GL_DEPTH_COMPONENT32F);
	g_buffer = EZCOGL::FBO_DepthTexture::create(textures, g_buffer_depth);

	Engine::get().on_window_resized.add_object(this, &Renderer::resize_framebuffer_internal);
	GL_CHECK_ERROR();
}

Renderer::~Renderer()
{
	Engine::get().on_window_resized.clear_object(this);
	glfwDestroyWindow(main_window);
}

void Renderer::initialize() const
{
	GL_CHECK_ERROR();
	STAT_DURATION("Initialize_Renderer");
	{
		STAT_DURATION("ImGui new frame");
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	{
		STAT_DURATION("Switch glfw context");
		if (glfwGetCurrentContext() != main_window)
			glfwMakeContextCurrent(main_window);
	}
	{
		STAT_DURATION("Handle_Events");
		glfwPollEvents();
	}
	EZCOGL::VAO::none()->bind();

	STAT_DURATION("Draw dock table");
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
	GL_CHECK_ERROR();
}

void Renderer::bind_g_buffers() const
{
	GL_CHECK_ERROR();
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
	GL_CHECK_ERROR();
}

void Renderer::submit() const
{
	GL_CHECK_ERROR();
	{
		if (!fullscreen) // Bind back buffer to display UI
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

		STAT_DURATION("ImGui Render");
		{
			STAT_DURATION("ImGui pre-render");
			ImGui::Render();
		}
		GL_CHECK_ERROR();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		while (glGetError() != GL_NO_ERROR);
		GL_CHECK_ERROR();
	}
	GL_CHECK_ERROR();

	STAT_DURATION("Swap_buffers");
	glfwSwapBuffers(main_window);
	GL_CHECK_ERROR();
}

bool Renderer::should_close() const
{
	return glfwWindowShouldClose(main_window);
}

void Renderer::switch_fullscreen()
{
	Engine::get().on_window_resized.clear_object(this);
	fullscreen = !fullscreen;
	if (fullscreen)
	{
		int w, h;
		glfwGetWindowSize(main_window, &w, &h);
		Engine::get().on_window_resized.add_object(this, &Renderer::resize_framebuffer_internal);
		resize_framebuffer_internal(nullptr, w, h);
	}
	on_fullscreen.execute(fullscreen);
}

void Renderer::resize_framebuffer_internal(GLFWwindow*, int x, int y)
{
	if (render_width == x && render_height == y || x <= 0 || y <= 0)
		return;

	render_width = x;
	render_height = y;

	framebuffer()->resize(x, y);
	on_resolution_changed.execute(x, y);
	Engine::get().get_world().get_camera()->viewport_res() = {x, y};
}

void Renderer::set_icon(const std::string& file_path)
{
	const EZCOGL::GLImage icon_image(file_path, false, 4);
	GLFWimage images[1];
	images[0].width = icon_image.width();
	images[0].height = icon_image.height();
	images[0].pixels = const_cast<unsigned char*>(icon_image.data());
	glfwSetWindowIcon(main_window, 1, images);
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

	main_window = glfwCreateWindow(default_window_res.x(), default_window_res.y(), "Planet Engine", nullptr, nullptr);
	if (!main_window) { std::cerr << "Failed to create Window!" << std::endl; }

	glfwMakeContextCurrent(main_window);

	bool err = gl3wInit() != 0;
	if (err) { std::cerr << "Failed to initialize OpenGL loader!" << std::endl; }

	glfwSwapInterval(1); // Enable vsync
	glfwSetWindowUserPointer(main_window, this);

	GL_CHECK_ERROR();
	IMGUI_CHECKVERSION();
	imgui_context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	io.Fonts->AddFontFromFileTTF("resources/fonts/Roboto-Medium.ttf", 16.f);
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;

	auto& style = ImGui::GetStyle();
	style.WindowRounding = 0;
	style.ScrollbarRounding = 0;
	style.FramePadding = ImVec2(10, 2);
	style.ItemSpacing = ImVec2(8, 3);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5);
	style.WindowPadding = ImVec2(4, 4);
	style.GrabMinSize = 12;
	style.IndentSpacing = 17;
	style.WindowBorderSize = 0;
	style.ChildBorderSize = 0;
	style.PopupBorderSize = 0;
	style.TabRounding = 0;
	style.ScrollbarSize = 10;
	style.FrameBorderSize = 1;
	style.PopupBorderSize = 1;
	
	style.Colors[ImGuiCol_Text] = COLOR_BR(232, 232, 232, 255);
	style.Colors[ImGuiCol_FrameBg] = COLOR(47, 47, 47, 255);
	style.Colors[ImGuiCol_FrameBgHovered] = COLOR(102, 102, 102, 255);
	style.Colors[ImGuiCol_FrameBgActive] = COLOR(85, 85, 85, 255);
	style.Colors[ImGuiCol_WindowBg] = COLOR(65, 65, 65, 255);
	style.Colors[ImGuiCol_Border] = COLOR(0, 0, 0, 255);
	style.Colors[ImGuiCol_Button] = COLOR(100, 100, 100, 255);
	style.Colors[ImGuiCol_ButtonHovered] = COLOR(140, 140, 140, 255);
	style.Colors[ImGuiCol_ButtonActive] = COLOR(78, 78, 78, 255);
	style.Colors[ImGuiCol_CheckMark] = COLOR(255, 222, 139, 255);
	style.Colors[ImGuiCol_SliderGrab] = COLOR(142, 142, 142, 255);
	style.Colors[ImGuiCol_SliderGrabActive] = COLOR(210, 210, 210, 255);
	style.Colors[ImGuiCol_Tab] = COLOR(0, 0, 0, 0);
	style.Colors[ImGuiCol_TabActive] = COLOR(86, 86, 86, 255);
	style.Colors[ImGuiCol_TabHovered] = COLOR(140, 140, 140, 255);
	style.Colors[ImGuiCol_TabUnfocusedActive] = COLOR(63, 63, 63, 255);
	style.Colors[ImGuiCol_TitleBgActive] = COLOR(48, 48, 48, 255);
	style.Colors[ImGuiCol_Header] = COLOR(73, 73, 73, 255);
	style.Colors[ImGuiCol_HeaderActive] = COLOR(48, 48, 48, 255);
	style.Colors[ImGuiCol_HeaderHovered] = COLOR(109, 109, 109, 255);
	style.Colors[ImGuiCol_TextSelectedBg] = COLOR(153, 153, 153, 255);
	style.Colors[ImGuiCol_NavHighlight] = COLOR(250, 203, 66, 255);
	style.Colors[ImGuiCol_DockingPreview] = COLOR(148, 148, 148, 223);
	style.Colors[ImGuiCol_PopupBg] = COLOR(70, 70, 70, 255);

#ifdef _WIN32
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	float xscale, yscale;
	glfwGetMonitorContentScale(monitor, &xscale, &yscale);
	if (xscale > 1 || yscale > 1)
	{
		const float highDPIscaleFactor = 1;
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(highDPIscaleFactor);
		ImGui::GetIO().FontGlobalScale = highDPIscaleFactor;
	}
#endif

	GL_CHECK_ERROR();
	ImGui_ImplGlfw_InitForOpenGL(main_window, true);

	GL_CHECK_ERROR();
	ImGui_ImplOpenGL3_Init(glsl_version);
	while (glGetError() != GL_NO_ERROR);

	GL_CHECK_ERROR();
	int nb_ext;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nb_ext);
	uniform_explicit_location_support = false;
	for (int i = 0; i < nb_ext; ++i)
	{
		const char* name = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
		if (std::string(name) == "GL_ARB_explicit_uniform_location")
			uniform_explicit_location_support = true;
	}
	GL_CHECK_ERROR();

	//necessary for non mutiple of 4 width image loading into texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	GL_CHECK_ERROR();

	if (!glfwExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		std::cerr << "GL_ARB_gpu_shader_fp64 is required but not supported" << std::endl;
		exit(EXIT_FAILURE);
	}

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

	GL_CHECK_ERROR();
}
