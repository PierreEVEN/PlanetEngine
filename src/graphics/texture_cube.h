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

	void set_data(int32_t w, int32_t h, uint32_t image_format, const void* data_top = nullptr,
	              const void* data_bottom = nullptr, const void* data_left = nullptr, const void* data_right = nullptr,
	              const void* data_front = nullptr, const void* data_back = nullptr);
	uint32_t id() override;
protected:
	TextureCube(std::string name, const TextureCreateInfos& params = {});
private:

	bool finished_loading = false;
	std::mutex load_mutex;
	std::thread async_load_thread;

	void* top = nullptr;
	void* bottom = nullptr;
	void* right = nullptr;
	void* left = nullptr;
	void* front = nullptr;
	void* back = nullptr;
};
