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

    static std::shared_ptr<Material> create(const std::string& name) {
        return std::shared_ptr<Material>(new Material(name));
    }

    /**
     * \brief Set model matrix (uniform mat4 model)
     */
    void set_model_transform(const Eigen::Affine3d& transformation);

    /**
     * \brief Set shader source file
     * \param compute_path
     */
    void load_from_source(const std::string& vertex_path, const std::string& fragment_path);

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

    int bind_texture(const std::shared_ptr<TextureBase>& texture, const std::string& binding_name) const;

private:
    Material(const std::string& name);

    // GL Handle
    uint32_t shader_program_id;

    // Source file paths
    ShaderSource vertex_source;
    ShaderSource fragment_source;

    void reload_internal();
    void mark_dirty() { is_dirty = true; }

    bool                                 is_dirty;
    std::unordered_map<std::string, int> bindings;
};
