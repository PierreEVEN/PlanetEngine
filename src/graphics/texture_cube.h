#pragma once

#include <array>

#include "texture_image.h"

class TextureCube : public TextureBase
{
public:
	~TextureCube() override;

	static std::shared_ptr<TextureCube> create(const std::string& name, const TextureCreateInfos& params = {})
	{
		return std::shared_ptr<TextureCube>(new TextureCube(name, params));
	}

	[[nodiscard]] int32_t depth() const override { return 6; }

	[[nodiscard]] uint32_t texture_type() const override;
	void bind(uint32_t unit = 0) override;

	void from_file(const std::string& file_top, const std::string& file_bottom, const std::string& file_right,
	               const std::string& file_left, const std::string& file_front, const std::string& file_back,
	               int force_nb_channel = 0);

	void set_data(int32_t w, int32_t h, uint32_t image_format, uint32_t index, const void* image_data = nullptr);
	uint32_t id() override;
protected:
	TextureCube(std::string name, const TextureCreateInfos& params = {});
private:

	std::array<bool, 6> finished_loading = { false };
	std::array<void*, 6> loaded_image_ptr = { nullptr };
	std::array<std::thread, 6> async_load_thread;
	uint8_t complete = 0;
};
