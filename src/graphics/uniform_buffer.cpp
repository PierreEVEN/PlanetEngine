

#include "uniform_buffer.h"

#include <GL/gl3w.h>

UniformBuffer::~UniformBuffer() {
    glDeleteBuffers(1, &ubo_id);
}

void UniformBuffer::set_data_raw(const void* data_ptr, size_t data_size) {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_id);
    glBufferData(GL_UNIFORM_BUFFER, data_size, data_ptr, GL_STATIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo_id, 0, data_size);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::UniformBuffer(const std::string& in_name) : name(in_name) {
    glGenBuffers(1, &ubo_id);
}
