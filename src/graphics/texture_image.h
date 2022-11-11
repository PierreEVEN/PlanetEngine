#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <GL/gl3w.h>

enum class TextureWrapping {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureMagFilter {
    Nearest,
    Linear
};

enum class TextureMinFilter {
    Nearest,
    Linear,
    MipMap_NearestNearest,
    MipMap_LinearNearest,
    MipMap_NearestLinear,
    MipMap_LinearLinear
};

enum class ImageFormat {
    R_U8 = GL_R8,
    RG_U8 = GL_RG8,
    RGB_U8 = GL_RGB8,
    RGBA_U8 = GL_RGBA8,
    R_F16 = GL_R16F,
    RG_F16 = GL_RG16F,
    RGB_F16 = GL_RGB16F,
    RGBA_F16 = GL_RGBA16F,
    R_F32 = GL_R32F,
    RG_F32 = GL_RG32F,
    RGB_F32 = GL_RGB32F,
    RGBA_F32 = GL_RGBA32F,
    Depth_F32 = GL_DEPTH_COMPONENT32F,
};

std::string image_format_to_string(ImageFormat format);

inline bool is_depth_format(ImageFormat format) {
    return format == ImageFormat::Depth_F32;
}

struct TextureCreateInfos {
    TextureWrapping  wrapping      = TextureWrapping::Repeat;
    TextureMagFilter filtering_mag = TextureMagFilter::Linear;
    TextureMinFilter filtering_min = TextureMinFilter::MipMap_LinearLinear;
    bool             srgb          = false;
};

class TextureBase {
public:
    virtual ~TextureBase();

    [[nodiscard]] virtual uint32_t width() const { return image_width; }
    [[nodiscard]] virtual uint32_t height() const { return image_height; }
    [[nodiscard]] virtual uint32_t depth() const { return image_depth; }
    [[nodiscard]] virtual uint32_t id() { return texture_id; }
    [[nodiscard]] ImageFormat      internal_format() const { return image_format; }
    virtual void                   bind(uint32_t unit = 0);

    const std::string        name;
    const TextureCreateInfos parameters;
protected:
    TextureBase(std::string name, int32_t in_texture_type, const TextureCreateInfos& params = {});
    uint32_t    image_width;
    uint32_t    image_height;
    uint32_t    image_depth;
    uint32_t    texture_id = 0;
    ImageFormat image_format;
    uint32_t    external_format;
    uint32_t    data_format;
    int32_t     texture_type = -1;
};

class Texture2D : public TextureBase {
public:
    ~Texture2D() override;

    static std::shared_ptr<Texture2D> create(const std::string& name, const TextureCreateInfos& params = {}) {
        return std::shared_ptr<Texture2D>(new Texture2D(name, params));
    }

    static std::shared_ptr<Texture2D> create(const std::string& name, const std::string& file, const TextureCreateInfos& params = {});


    [[nodiscard]] uint32_t depth() const override { return 1; }
    void                   set_data(uint32_t w, uint32_t h, ImageFormat image_format, const void* data_ptr = nullptr);

    [[nodiscard]] uint32_t id() override;

protected:
    bool        finished_loading = false;
    std::mutex  load_mutex;
    std::thread async_load_thread;
    void*       loading_image_ptr = nullptr;

    Texture2D(std::string name, const TextureCreateInfos& params = {});
    Texture2D(std::string name, const std::string& file, const TextureCreateInfos& params = {});
};
