#include "texture_image.h"

#include "engine/asset_manager.h"
#include "engine/engine.h"
#include <algorithm>
#include <functional>
#include <texture2d.h>
#include <GL/gl3w.h>

#include "utils/gl_tools.h"
#include "utils/profiler.h"

std::string image_format_to_string(ImageFormat format) {
    switch (format) {
    case ImageFormat::R_U8:
        return "R_U8";
    case ImageFormat::RG_U8:
        return "RG_U8";
    case ImageFormat::RGB_U8:
        return "RGB_U8";
    case ImageFormat::RGBA_U8:
        return "RGBA_U8";
    case ImageFormat::R_F16:
        return "R_F16";
    case ImageFormat::RG_F16:
        return "RG_F16";
    case ImageFormat::RGB_F16:
        return "RGB_F16";
    case ImageFormat::RGBA_F16:
        return "RGBA_F16";
    case ImageFormat::R_F32:
        return "R_F32";
    case ImageFormat::RG_F32:
        return "RG_F32";
    case ImageFormat::RGB_F32:
        return "RGB_F32";
    case ImageFormat::RGBA_F32:
        return "RGBA_F32";
    case ImageFormat::Depth_F32:
        return "Depth_F32";
    default:
        return "undefined format";
    }
}

TextureBase::~TextureBase() {
    glDeleteTextures(1, &texture_id);
}

std::shared_ptr<TextureBase> TextureBase::create(const std::string& name, const TextureCreateInfos& params) {
    return std::shared_ptr<TextureBase>(new TextureBase(name, params));
}

void TextureBase::bind(uint32_t unit) {
    if (id() != 0) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(texture_type(), id());
        GL_CHECK_ERROR();
    }
}

uint32_t TextureBase::texture_type() const {
    return GL_TEXTURE_2D;
}

TextureBase::TextureBase(std::string in_name, const TextureCreateInfos& params)
    : name(std::move(in_name)), parameters(params) {
    GL_CHECK_ERROR();
    glGenTextures(1, &texture_id);

    bind();

    int min_filter = -1;
    switch (params.filtering_min) {
    case TextureMinFilter::Nearest:
        min_filter = GL_NEAREST;
        break;
    case TextureMinFilter::Linear:
        min_filter = GL_LINEAR;
        break;
    case TextureMinFilter::MipMap_NearestNearest:
        min_filter = GL_NEAREST_MIPMAP_NEAREST;
        break;
    case TextureMinFilter::MipMap_LinearNearest:
        min_filter = GL_LINEAR_MIPMAP_NEAREST;
        break;
    case TextureMinFilter::MipMap_NearestLinear:
        min_filter = GL_NEAREST_MIPMAP_LINEAR;
        break;
    case TextureMinFilter::MipMap_LinearLinear:
        min_filter = GL_LINEAR_MIPMAP_LINEAR;
        break;
    }

    int mag_filter = -1;
    switch (params.filtering_mag) {
    case TextureMagFilter::Nearest:
        mag_filter = GL_NEAREST;
        break;
    case TextureMagFilter::Linear:
        mag_filter = GL_LINEAR;
        break;
    }

    int wrapping = -1;
    switch (params.wrapping) {
    case TextureWrapping::Repeat:
        wrapping = GL_REPEAT;
        break;
    case TextureWrapping::MirroredRepeat:
        wrapping = GL_MIRRORED_REPEAT;
        break;
    case TextureWrapping::ClampToEdge:
        wrapping = GL_CLAMP_TO_EDGE;
        break;
    case TextureWrapping::ClampToBorder:
        wrapping = GL_CLAMP_TO_BORDER;
        break;
    }

    glTexParameteri(texture_type(), GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(texture_type(), GL_TEXTURE_MAG_FILTER, mag_filter);

    glTexParameteri(texture_type(), GL_TEXTURE_WRAP_S, wrapping);
    glTexParameteri(texture_type(), GL_TEXTURE_WRAP_T, wrapping);
    glTexParameteri(texture_type(), GL_TEXTURE_WRAP_R, wrapping);
    GL_CHECK_ERROR();
}

Texture2D::~Texture2D() {
    auto& textures = Engine::get().get_asset_manager().textures;
    textures.erase(std::find(textures.begin(), textures.end(), this));
    if (async_load_thread.joinable())
        async_load_thread.join();

    if (loading_image_ptr)
        delete static_cast<EZCOGL::GLImage*>(loading_image_ptr);
}

static std::unordered_map<std::string, std::shared_ptr<Texture2D>> texture_registry;

std::shared_ptr<Texture2D> Texture2D::create(const std::string& name, const std::string& file, const TextureCreateInfos& params) {
    const auto& texture = texture_registry.find(file);
    if (texture != texture_registry.end())
        return texture->second;
    
    return texture_registry[file] = std::shared_ptr<Texture2D>(new Texture2D(name, file, params));
}

void Texture2D::set_data(uint32_t w, uint32_t h, ImageFormat in_image_format, const void* data_ptr) {
    image_format    = in_image_format;
    auto tf         = EZCOGL::Texture::texture_formats[static_cast<int>(image_format)];
    external_format = tf.first;
    image_width     = w;
    image_height    = h;
    data_format     = tf.second;
    GL_CHECK_ERROR();
    bind();
    GL_CHECK_ERROR();
    glTexImage2D(texture_type(), 0, data_ptr ? (parameters.srgb ? GL_SRGB : static_cast<int>(in_image_format)) : static_cast<int>(image_format), w, h, 0, external_format,
                 data_format,
                 (w * h > 0) ? data_ptr : nullptr);
    GL_CHECK_ERROR();
    glBindTexture(texture_type(), 0);

    GL_CHECK_ERROR();
}

uint32_t Texture2D::id() {
    if (finished_loading) {
        STAT_ACTION("set texture data [" + name + "]");
        std::lock_guard lock_guard(load_mutex);
        finished_loading = false;

        const auto* image = static_cast<EZCOGL::GLImage*>(loading_image_ptr);

        switch (image->depth()) {
        case 1:
            set_data(image->width(), image->height(), ImageFormat::R_U8, image->data());
            break;
        case 3:
            set_data(image->width(), image->height(), ImageFormat::RGB_U8, image->data());
            break;
        case 4:
            set_data(image->width(), image->height(), ImageFormat::RGBA_U8, image->data());
            break;
        default:
            delete image;
            loading_image_ptr = nullptr;
            return 0;
        }

        bind();
        glGenerateMipmap(texture_type());
        glBindTexture(texture_type(), 0);

        GL_CHECK_ERROR();
        delete image;
        loading_image_ptr = nullptr;
    }

    return TextureBase::id();
}

Texture2D::Texture2D(std::string name, const TextureCreateInfos& params)
    : TextureBase(name, params) {
    Engine::get().get_asset_manager().textures.emplace_back(this);

}

Texture2D::Texture2D(std::string name, const std::string& file, const TextureCreateInfos& params)
    : Texture2D(name, params) {

    if (async_load_thread.joinable())
        async_load_thread.join();

    async_load_thread = std::thread([&, file] {
        std::lock_guard lock_guard(load_mutex);
        STAT_ACTION("Load texture data [" + file + "]");
        finished_loading  = false;
        loading_image_ptr = new EZCOGL::GLImage(file, EZCOGL::Texture::flip_y_on_load);
        finished_loading  = true;
    });
}
