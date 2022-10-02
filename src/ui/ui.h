#pragma once

#include <memory>
#include <string>

class World;
class Renderer;

namespace EZCOGL
{
	class FBO_DepthTexture;
}

namespace ui
{
	void draw();
}

class ImGuiWindow
{
	friend void ui::draw();
public:
	ImGuiWindow() = default;
	virtual ~ImGuiWindow() = default;
	virtual void draw() = 0;

	template <typename Window_T, typename...Args>
	static std::shared_ptr<Window_T> create_window(Args ... args)
	{
		const auto ptr = std::make_shared<Window_T>(std::forward<Args>(args)...);
		ptr->window_id = make_window_id();
		register_window_internal(ptr);
		return ptr;
	}

	[[nodiscard]] std::string& name()
	{
		return window_name;
	}

	void close()
	{
		is_open = false;
	}
protected:
	std::string window_name;
private:
	static size_t make_window_id();
	static void register_window_internal(std::shared_ptr<ImGuiWindow> window);
	static void draw_all();
	void draw_internal();
	size_t window_id;
	bool is_open = true;
};
