#pragma once
#include <memory>
#include <mutex>
#include <string>

enum class TextureWrapping
{
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder
};

enum class TextureMagFilter
{
	Nearest,
	Linear
};

enum class TextureMinFilter
{
	Nearest,
	Linear,
	MipMap_NearestNearest,
	MipMap_LinearNearest,
	MipMap_NearestLinear,
	MipMap_LinearLinear
};

struct TextureCreateInfos
{
	TextureWrapping wrapping = TextureWrapping::Repeat;
	TextureMagFilter filtering_mag = TextureMagFilter::Linear;
	TextureMinFilter filtering_min = TextureMinFilter::MipMap_LinearLinear;
};

class TextureBase
{
public:
	virtual ~TextureBase();

	static std::shared_ptr<TextureBase> create(const std::string& name, const TextureCreateInfos& params = {});

	[[nodiscard]] int32_t width() const { return image_width; }
	[[nodiscard]] int32_t height() const { return image_height; }
	[[nodiscard]] virtual int32_t depth() const { return image_depth; }
	[[nodiscard]] virtual uint32_t id() { return texture_id; }
	[[nodiscard]] uint32_t internal_format() const { return image_format; }
	const std::string name;
	void bind(uint32_t unit = 0);
	bool is_depth() const;
protected:
	TextureBase(std::string name, const TextureCreateInfos& params = {});
	int32_t image_width;
	int32_t image_height;
	int32_t image_depth;
	uint32_t texture_id;
	uint32_t image_format;
	uint32_t external_format;
	uint32_t data_format;
};

class Texture2D : public TextureBase
{
public:
	~Texture2D() override
	{
		if (async_load_thread.joinable())
			async_load_thread.join();
	}

	static std::shared_ptr<Texture2D> create(const std::string& name, const TextureCreateInfos& params = {})
	{
		return std::shared_ptr<Texture2D>(new Texture2D(name, params));
	}

	[[nodiscard]] int32_t depth() const override { return 1; }
	void from_file(const std::string& filename, int force_nb_channel = 0);
	void set_data(int32_t w, int32_t h, uint32_t image_format, const void* data_ptr = nullptr);

	[[nodiscard]] uint32_t id() override;

protected:
	bool finished_loading = false;
	std::mutex load_mutex;
	std::thread async_load_thread;
	void* loading_image_ptr = nullptr;

	Texture2D(std::string name, const TextureCreateInfos& params = {}) : TextureBase(name, params)
	{
	}
};
