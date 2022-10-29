

#include "storage_buffer.h"

#include <GL/gl3w.h>

StorageBuffer::~StorageBuffer()
{
	glDeleteBuffers(1, &ssbo_id);
}

void StorageBuffer::set_data_raw(const void* data_ptr, size_t data_size)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_id);
	glBufferData(GL_SHADER_STORAGE_BUFFER, data_size, data_ptr, GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

StorageBuffer::StorageBuffer(const std::string& in_name)
{
    glGenBuffers(1, &ssbo_id);
}
