

#include "texture_image.h"

class TextureCube : public TextureBase
{
public:
	virtual ~TextureCube() = default;

	static std::shared_ptr<TextureCube> create(const std::string& name, const TextureCreateInfos& params = {})
	{
		return std::shared_ptr<TextureCube>(new TextureCube(name, params));
	}

	[[nodiscard]] int32_t depth() const override { return 6; }

	bool from_file(const std::string& file_x, const std::string& file_y, const std::string& file_z,
		const std::string& file_mx, const std::string& file_my, const std::string& file_mz,
		int force_nb_channel = 0);

	void set_data(int32_t w, int32_t h, uint32_t image_format, const void* data_ptr = nullptr);
protected:
	TextureCube(std::string name, const TextureCreateInfos& params = {}) : TextureBase(name, params)
	{
	}
};

