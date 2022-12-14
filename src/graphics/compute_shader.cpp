#include "compute_shader.h"

#include <shader_program.h>
#include <GL/gl3w.h>

#include "texture_image.h"
#include "engine/asset_manager.h"
#include "engine/engine.h"
#include "utils/gl_tools.h"

ComputeShader::~ComputeShader() {
    auto& computes = Engine::get().get_asset_manager().compute_shaders;
    computes.erase(std::find(computes.begin(), computes.end(), this));
    glDeleteProgram(compute_shader_id);
}

static std::unordered_map<std::string, std::shared_ptr<ComputeShader>> compute_registry;

std::shared_ptr<ComputeShader> ComputeShader::create(const std::string& name, const std::string& compute_path) {
    const auto& compute = compute_registry.find(compute_path);
    if (compute != compute_registry.end())
        return compute->second;
    return compute_registry[compute_path] = std::shared_ptr<ComputeShader>(new ComputeShader(name, compute_path));
}

void ComputeShader::check_updates() {
    if (!auto_reload)
        return;
    compute_source.check_update();
    if (is_dirty)
        reload_internal();
}

void ComputeShader::bind() {
    GL_CHECK_ERROR();
    if (is_dirty)
        reload_internal();

    if (compilation_error)
        return;

    glUseProgram(compute_shader_id);
    GL_CHECK_ERROR();
}

void ComputeShader::execute(int x, int y, int z) const {
    if (compilation_error)
        return;

    GL_CHECK_ERROR();
    glDispatchCompute(x, y, z);
    GL_CHECK_ERROR();
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    GL_CHECK_ERROR();
}

void ComputeShader::bind_texture(const std::shared_ptr<TextureBase>& texture, BindingMode mode, int32_t binding) {
    int in_out = 0;
    switch (mode) {
    case BindingMode::In:
        in_out = GL_READ_ONLY;
        break;
    case BindingMode::Out:
        in_out = GL_WRITE_ONLY;
        break;
    case BindingMode::InOut:
        in_out = GL_READ_WRITE;
        break;
    }
    glBindImageTexture(binding, texture->id(), 0, GL_FALSE, 0, in_out, static_cast<GLenum>(texture->internal_format()));
}

ComputeShader::ComputeShader(const std::string& in_name, const std::string& compute_path)
    : name(in_name), compute_shader_id(0) {
    Engine::get().get_asset_manager().compute_shaders.emplace_back(this);

    compute_source.set_source_path(compute_path);
    compute_source.on_data_changed.add_object(this, &ComputeShader::mark_dirty);
    mark_dirty();
}

void ComputeShader::reload_internal() {
    GL_CHECK_ERROR();
    // Destroy existing handle
    if (compute_shader_id)
        glDeleteProgram(compute_shader_id);

    compute_shader_id = glCreateProgram();

    compilation_error.reset();
    std::string fragment_error;
    size_t      fragment_error_line;

    // Load shader
    GL_CHECK_ERROR();
    std::unique_ptr<EZCOGL::Shader> program_compute = std::make_unique<EZCOGL::Shader>(GL_COMPUTE_SHADER);
    GL_CHECK_ERROR();
    // Compile shader
    if (!program_compute->compile(compute_source.get_source_code(), name, fragment_error, fragment_error_line)) {
        // Error occurred during compilation
        size_t local_line;
        compilation_error = {
            .error = fragment_error,
            .line = 0,
            .file = compute_source.get_file_at_line(fragment_error_line, local_line),
            .is_fragment = true,
        };
        compilation_error->line = local_line;
        is_dirty                = false;
        return;
    }

    // Link shader
    GL_CHECK_ERROR();
    glAttachShader(compute_shader_id, program_compute->shaderId());
    glLinkProgram(compute_shader_id);
    glDetachShader(compute_shader_id, program_compute->shaderId());
    GL_CHECK_ERROR();

    // Check link errors
    int info_log_length = 0;
    glGetProgramiv(compute_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 1) {
        char* info_log      = new char[info_log_length];
        int   chars_written = 0;
        glGetProgramInfoLog(compute_shader_id, info_log_length, &chars_written, info_log);
        compilation_error = {
            .error = info_log,
            .line = 0,
            .file = "",
            .is_fragment = false,
        };
        delete[] info_log;
        glDeleteProgram(compute_shader_id);
        compute_shader_id = 0;
        is_dirty          = false;
        return;
    }
    if (compilation_error) {
        glDeleteProgram(compute_shader_id);
        compute_shader_id = 0;
        glUseProgram(0);
        GL_CHECK_ERROR();
        return;
    }
    GL_CHECK_ERROR();

    // Make world data uniform accessible by any shader
    const int world_data_id = glGetUniformBlockIndex(compute_shader_id, "WorldData");
    if (world_data_id >= 0)
        glUniformBlockBinding(compute_shader_id, world_data_id, 0);
    GL_CHECK_ERROR();

    // Get bindings (to avoid openGL calls later)
    int uniform_count;
    glGetProgramiv(compute_shader_id, GL_ACTIVE_UNIFORMS, &uniform_count);
    for (int i = 0; i < uniform_count; ++i) {
        GLchar uniform_name[256];
        int    length;
        int    type_size;
        GLenum type_type;
        glGetActiveUniform(compute_shader_id, static_cast<GLuint>(i), 256, &length, &type_size, &type_type,
                           uniform_name);
    }
    GL_CHECK_ERROR();

    is_dirty = false;
    on_reload.execute();
}
