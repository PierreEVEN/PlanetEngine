#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "shader_source.h"

DECLARE_DELEGATE_MULTICAST(EventReloadShader);

class TextureBase;

enum class BindingMode {
    // Read only
    In,
    // Write only
    Out,
    // Read and write
    InOut
};

class ComputeShader {
public:
    ~ComputeShader();

    static std::shared_ptr<ComputeShader> create(const std::string& name, const std::string& compute_path);
    
    /**
     * \brief Check if source file have been changed.
     */
    void check_updates();

    /**
     * \brief Shader program source code
     * \return
     */
    [[nodiscard]] ShaderSource& get_program_source() { return compute_source; }

    /**
     * \brief OpenGL Handle
     */
    [[nodiscard]] uint32_t program_id() const { return compute_shader_id; }

    const std::string                   name;
    std::optional<CompilationErrorInfo> compilation_error;
    bool                                auto_reload = false;

    /**
     * \brief Bind shader
     */
    void bind();

    /**
     * \brief Run compute shader program.
     */
    void execute(int x, int y, int z) const;

    /**
     * \brief Bind given texture to this shader
     * \param mode hove it is used
     */
    void bind_texture(const std::shared_ptr<TextureBase>& texture, BindingMode mode, int32_t binding);

    /**
     * \brief Shader source code have been modified
     */
    EventReloadShader on_reload;
private:
    ComputeShader(const std::string& in_name, const std::string& compute_path);
    ShaderSource compute_source;

    void     reload_internal();
    void     mark_dirty() { is_dirty = true; }
    bool     is_dirty = true;
    uint32_t compute_shader_id;
};
