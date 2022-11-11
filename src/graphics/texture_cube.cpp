#include "texture_cube.h"

#include <imgui.h>
#include <texture2d.h>

#include "utils/gl_tools.h"
#include "utils/profiler.h"

TextureCube::~TextureCube() {
    for (auto& thread : async_load_thread)
        if (thread.joinable())
            thread.join();
    for (const auto& ptr : loaded_image_ptr)
        if (ptr)
            delete static_cast<EZCOGL::GLImage*>(ptr);
}

void TextureCube::from_file(const std::string& file_top, const std::string&  file_bottom, const std::string& file_right,
                            const std::string& file_left, const std::string& file_front, const std::string&  file_back,
                            int                force_nb_channel) {
    for (auto& thread : async_load_thread)
        if (thread.joinable())
            thread.join();

    const auto files = std::vector{file_right, file_left, file_top, file_bottom, file_front, file_back};
    for (size_t i = 0; i < files.size(); ++i) {
        async_load_thread[i] = std::thread(
            [&, files, i, force_nb_channel] {
                STAT_ACTION("load cubemap [" + files[i] + "]::" + std::to_string(i));
                finished_loading[i] = false;
                loaded_image_ptr[i] = new EZCOGL::GLImage(files[i], EZCOGL::Texture::flip_y_on_load, force_nb_channel);
                if (!static_cast<EZCOGL::GLImage*>(loaded_image_ptr[i])->data())
                    std::cerr << "failed to load " << file_top.c_str() << std::endl;
                finished_loading[i] = true;
            });
    }
}

void TextureCube::set_data(int32_t w, int32_t h, ImageFormat in_image_format, uint32_t index, const void* image_data) {
    STAT_ACTION("set cubemap data [" + name + "]::" + std::to_string(index));
    image_format    = in_image_format;
    const auto tf   = EZCOGL::Texture::texture_formats[static_cast<int>(image_format)];
    external_format = tf.first;
    image_width     = w;
    image_height    = h;
    data_format     = tf.second;
    GL_CHECK_ERROR();
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
    GL_CHECK_ERROR();
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, GL_SRGB, w, h, 0, external_format, data_format,
                 (w * h > 0) ? image_data : nullptr);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    GL_CHECK_ERROR();
}

uint32_t TextureCube::id() {
    for (size_t i = 0; i < finished_loading.size(); ++i) {
        if (finished_loading[i]) {
            finished_loading[i] = false;
            const auto* image   = static_cast<EZCOGL::GLImage*>(loaded_image_ptr[i]);
            switch (image->depth()) {
            case 1:
                set_data(image->width(), image->height(), ImageFormat::R_U8, static_cast<uint32_t>(i), image->data());
                break;
            case 3:
                set_data(image->width(), image->height(), ImageFormat::RGB_U8, static_cast<uint32_t>(i), image->data());
                break;
            case 4:
                set_data(image->width(), image->height(), ImageFormat::RGBA_U8, static_cast<uint32_t>(i), image->data());
                break;
            default:
                break;
            }
            delete image;
            loaded_image_ptr[i] = nullptr;
            complete |= 1 << i;
            GL_CHECK_ERROR();
            if (complete == 0b111111) {
                STAT_ACTION("Rebuild cubemap mipmaps : [" + name + "]");
                GL_CHECK_ERROR();
                glBindTexture(GL_TEXTURE_CUBE_MAP, TextureBase::id());
                GL_CHECK_ERROR();
                // glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
                // Generating mipmaps on cubemaps doesn't works i don't know why ???
                GL_CHECK_ERROR();
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                GL_CHECK_ERROR();
            }
            GL_CHECK_ERROR();
        }
    }

    return
        TextureBase::id();
}

static TextureCreateInfos cubemap_params(TextureCreateInfos params) {
    params.wrapping = TextureWrapping::ClampToEdge;
    return params;
}

TextureCube::TextureCube(std::string name, const TextureCreateInfos& params)
    : TextureBase(name, GL_TEXTURE_CUBE_MAP, cubemap_params(params)) {
}
