#include "material.h"

#include <filesystem>
#include <GL/gl3w.h>

#include <shader_program.h>

#include "texture_image.h"
#include "engine/asset_manager.h"
#include "engine/engine.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"


int Material::binding(const std::string& binding_name) const {
    const auto binding = bindings.find(binding_name);
    if (binding != bindings.end())
        return binding->second;
    return -1;
}

bool Material::set_texture(const std::string& bind_name, const std::shared_ptr<TextureBase>& texture) const {
    const int bp = binding(bind_name);
    if (bp > 0) {
        glUniform1i(bp, bp);
        GL_CHECK_ERROR();
        texture->bind(bp);
        return true;
    }
    return false;
}

bool Material::set_float(const std::string& bind_name, float value) const {
    const int bp = binding(bind_name);
    if (bp >= 0) {
        glUniform1f(bp, value);
        GL_CHECK_ERROR();
        return true;
    }
    return false;
}

bool Material::set_int(const std::string& bind_name, int value) const {
    const int bp = binding(bind_name);
    if (bp >= 0) {
        glUniform1i(bp, value);
        GL_CHECK_ERROR();
        return true;
    }
    return false;
}

bool Material::set_rotation(const std::string& bind_name, const Eigen::Quaterniond& value) const {
    const int bp = binding(bind_name);
    if (bp >= 0) {
        glUniformMatrix3fv(bp, 1, false, value.cast<float>().matrix().data());
        GL_CHECK_ERROR();
        return true;
    }
    return false;
}

bool Material::set_transform(const std::string& bind_name, const Eigen::Affine3d& value) const {
    const int bp = binding(bind_name);
    if (bp >= 0) {
        glUniformMatrix4fv(bp, 1, false, value.cast<float>().matrix().data());
        GL_CHECK_ERROR();
        return true;
    }
    return false;
}

bool Material::set_vec4(const std::string& bind_name, const Eigen::Vector4f& value) const {
    const int bp = binding(bind_name);
    if (bp >= 0) {
        glUniform4fv(bp, 1, value.data());
        GL_CHECK_ERROR();
        return true;
    }
    return false;
}

bool Material::set_vec3(const std::string& bind_name, const Eigen::Vector3f& value) const {
    const int bp = binding(bind_name);
    if (bp >= 0) {
        glUniform3fv(bp, 1, value.data());
        GL_CHECK_ERROR();
        return true;
    }
    return false;
}

Material::Material(const std::string& in_name, const std::string& vertex_path, const std::string& fragment_path, const std::optional<std::string>& geometry_path)
    : name(in_name), shader_program_id(0) {
    Engine::get().get_asset_manager().materials.emplace_back(this);

    vertex_source.set_source_path(vertex_path);
    vertex_source.on_data_changed.add_object(this, &Material::mark_dirty);

    if (geometry_path) {
        geometry_source = ShaderSource();
        geometry_source->set_source_path(*geometry_path);
        geometry_source->on_data_changed.add_object(this, &Material::mark_dirty);
    }

    fragment_source.set_source_path(fragment_path);
    fragment_source.on_data_changed.add_object(this, &Material::mark_dirty);

    mark_dirty();

}

void Material::reload_internal() {
    STAT_ACTION("Compile shader [" + name + "]");
    GL_CHECK_ERROR();
    if (shader_program_id != 0)
        glDeleteProgram(shader_program_id);

    GL_CHECK_ERROR();
    compilation_error.reset();
    bindings.clear();
    // Compile shader
    shader_program_id = glCreateProgram();

    const std::unique_ptr<EZCOGL::Shader> program_vertex = std::make_unique<EZCOGL::Shader>(GL_VERTEX_SHADER);
    std::string                           vertex_error;
    size_t                                vertex_error_line;
    if (!program_vertex->compile(vertex_source.get_source_code(), name, vertex_error, vertex_error_line)) {
        size_t local_line;
        compilation_error = {
            .error = vertex_error,
            .line = 0,
            .file = vertex_source.get_file_at_line(vertex_error_line, local_line),
            .is_fragment = false,
        };
        compilation_error->line = local_line;
        glDeleteProgram(shader_program_id);
        shader_program_id = 0;
        is_dirty          = false;
        return;
    }
    GL_CHECK_ERROR();

    std::unique_ptr<EZCOGL::Shader> program_geometry = nullptr;
    if (geometry_source) {
        program_geometry = std::make_unique<EZCOGL::Shader>(GL_GEOMETRY_SHADER);
        std::string geometry_error;
        size_t      geometry_error_line;
        if (!program_geometry->compile(geometry_source->get_source_code(), name, geometry_error, geometry_error_line)) {
            size_t local_line;
            compilation_error = {
                .error = geometry_error,
                .line = 0,
                .file = geometry_source->get_file_at_line(geometry_error_line, local_line),
                .is_fragment = false,
            };
            compilation_error->line = local_line;
            glDeleteProgram(shader_program_id);
            shader_program_id = 0;
            is_dirty          = false;
            return;
        }
    }

    std::string                     fragment_error;
    size_t                          fragment_error_line;
    std::unique_ptr<EZCOGL::Shader> program_fragment = std::make_unique<EZCOGL::Shader>(GL_FRAGMENT_SHADER);
    if (!program_fragment->compile(fragment_source.get_source_code(), name, fragment_error, fragment_error_line)) {
        size_t local_line;
        compilation_error = {
            .error = fragment_error,
            .line = 0,
            .file = fragment_source.get_file_at_line(fragment_error_line, local_line),
            .is_fragment = true,
        };
        compilation_error->line = local_line;
        glDeleteProgram(shader_program_id);
        shader_program_id = 0;
        is_dirty          = false;
        return;
    }

    // Link shader
    glAttachShader(shader_program_id, program_vertex->shaderId());
    glAttachShader(shader_program_id, program_fragment->shaderId());
    if (program_geometry)
        glAttachShader(shader_program_id, program_geometry->shaderId());

    glLinkProgram(shader_program_id);

    if (program_geometry)
        glDetachShader(shader_program_id, program_geometry->shaderId());
    glDetachShader(shader_program_id, program_vertex->shaderId());
    glDetachShader(shader_program_id, program_fragment->shaderId());

    // Check link errors
    GL_CHECK_ERROR();
    int infologLength = 0;
    glGetProgramiv(shader_program_id, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 1) {
        char* infoLog      = new char[infologLength];
        int   charsWritten = 0;
        glGetProgramInfoLog(shader_program_id, infologLength, &charsWritten, infoLog);
        compilation_error = {
            .error = infoLog,
            .line = 0,
            .file = "",
            .is_fragment = false,
        };
        delete[] infoLog;
        glDeleteProgram(shader_program_id);
        shader_program_id = 0;
        is_dirty          = false;
        return;
    }
    if (compilation_error) {
        glDeleteProgram(shader_program_id);
        shader_program_id = 0;
        glUseProgram(0);
    }
    GL_CHECK_ERROR();

    // Make world data uniform accessible by any shader
    const int world_data_id = glGetUniformBlockIndex(shader_program_id, "WorldData");
    if (world_data_id >= 0)
        glUniformBlockBinding(shader_program_id, world_data_id, 0);

    // Get bindings (to avoid openGL calls later)
    GL_CHECK_ERROR();
    int uniform_count;
    glGetProgramiv(shader_program_id, GL_ACTIVE_UNIFORMS, &uniform_count);
    for (int i = 0; i < uniform_count; ++i) {
        GLchar uniform_name[256];
        int    length;
        int    type_size;
        GLenum type_type;
        glGetActiveUniform(shader_program_id, static_cast<GLuint>(i), 256, &length, &type_size, &type_type,
                           uniform_name);
        bindings.insert({uniform_name, glGetUniformLocation(shader_program_id, uniform_name)});
    }
    GL_CHECK_ERROR();

    is_dirty = false;
}

Material::~Material() {
    auto& materials = Engine::get().get_asset_manager().materials;
    materials.erase(std::find(materials.begin(), materials.end(), this));
    glDeleteProgram(shader_program_id);
}

static std::unordered_map<std::string, std::shared_ptr<Material>> material_registry;

std::shared_ptr<Material> Material::create(const std::string&                name, const std::string& vertex_path, const std::string& fragment_path,
                                           const std::optional<std::string>& geometry_path) {
    const std::string hash     = vertex_path + fragment_path + (geometry_path ? *geometry_path : "");
    const auto&       material = material_registry.find(hash);
    if (material != material_registry.end())
        return material->second;

    return material_registry[hash] = std::shared_ptr<Material>(new Material(name, vertex_path, fragment_path, geometry_path));
}

bool Material::bind() {
    GL_CHECK_ERROR();
    if (is_dirty)
        reload_internal();

    if (compilation_error)
        return false;

    glUseProgram(shader_program_id);
    GL_CHECK_ERROR();
    return true;
}

void Material::check_updates() {
    if (!auto_reload)
        return;

    vertex_source.check_update();
    fragment_source.check_update();
    if (geometry_source)
        geometry_source->check_update();

    if (is_dirty)
        reload_internal();
}
