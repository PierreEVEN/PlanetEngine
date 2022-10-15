#pragma once

#include <memory>
#include <GL/gl3w.h>

namespace EZCOGL
{
	class TextureInterface
	{
	public:
		using SP = std::shared_ptr<TextureInterface>;

		virtual void init_interface(GLint internal)
		{
		}

		virtual int id_interface() = 0;

		virtual void resize_interface(GLsizei w, GLsizei h)
		{
		}

		virtual GLsizei width_interface() const = 0;
		virtual GLsizei height_interface() const { return 0; }

		virtual void set_data_interface(int32_t w, int32_t h, uint32_t image_format, const void* data_ptr = nullptr)
		{
		}
	};
}