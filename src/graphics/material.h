#pragma once

#include <optional>
#include <GL/gl3w.h>
#include <unordered_map>
#include <Eigen/Dense>

#include "shader_source.h"


class TextureBase;

class Material final {
public:
    ~Material();

    static std::shared_ptr<Material> create(const std::string&                name, const std::string& vertex_path, const std::string& fragment_path,
                                            const std::optional<std::string>& geometry_path = {});

    /**
     * \brief Check if source file have been changed.
     */
    void check_updates();

    /**
     * \brief Shader program source code for vertex stage
     * \return
     */
    [[nodiscard]] const ShaderSource& get_vertex_source() const { return vertex_source; }

    /**
     * \brief Shader program source code for fragment stage
     * \return
     */
    [[nodiscard]] const ShaderSource& get_fragment_source() const { return fragment_source; }

    /**
     * \brief Shader program source code for geometry stage
     * \return
     */
    [[nodiscard]] const std::optional<ShaderSource>& get_geometry_source() const { return geometry_source; }

    /**
     * \brief OpenGL Handle
     */
    [[nodiscard]] uint32_t program_id() const { return shader_program_id; }

    /**
     * \brief Bind shader. Return false if shader could not be bound. (ie : compilation error)
     */
    bool bind();

    /**
     * \brief Enable or disable hot reload for this material
     */
    bool              auto_reload = false;
    const std::string name;

    /**
     * \brief Last compilation error information.
     */
    std::optional<CompilationErrorInfo> compilation_error;

    /**
     * \brief Get binding index from name.
     */
    [[nodiscard]] int binding(const std::string& binding_name) const;

    bool set_float(const std::string& bind_name, float value) const;
    bool set_int(const std::string& bind_name, int value) const;
    bool set_rotation(const std::string& bind_name, const Eigen::Quaterniond& value) const;
    bool set_transform(const std::string& bind_name, const Eigen::Affine3d& value) const;
    bool set_vec4(const std::string& binding, const Eigen::Vector4d& value) const { return set_vec4(binding, static_cast<Eigen::Vector4f>(value.cast<float>())); }
    bool set_vec4(const std::string& bind_name, const Eigen::Vector4f& value) const;
    bool set_vec3(const std::string& binding, const Eigen::Vector3d& value) const { return set_vec3(binding, static_cast<Eigen::Vector3f>(value.cast<float>())); }
    bool set_vec3(const std::string& bind_name, const Eigen::Vector3f& value) const;
    bool set_texture(const std::string& bind_name, const std::shared_ptr<TextureBase>& texture) const;

    /**
     * \brief Set model matrix (uniform mat4 model)
     */
    bool set_model_transform(const Eigen::Affine3d& transformation) { return set_transform("model", transformation); }

private:
    Material(const std::string& name, const std::string& vertex_path, const std::string& fragment_path, const std::optional<std::string>& geometry_path = {});

    // GL Handle
    uint32_t shader_program_id;

    // Source file paths
    ShaderSource                vertex_source;
    ShaderSource                fragment_source;
    std::optional<ShaderSource> geometry_source;

    void reload_internal();
    void mark_dirty() { is_dirty = true; }

    bool                                 is_dirty;
    std::unordered_map<std::string, int> bindings;
};
