#pragma once
#include <string>

#include "utils/event_manager.h"

class ShaderSource;
DECLARE_DELEGATE_MULTICAST(Event_ShaderFileUpdate);

struct CompilationErrorInfo {
    std::string error;
    size_t      line;
    std::string file;
    bool        is_fragment;
};

/**
 * \brief Represent a piece of shader code.
 * Could be an include directive, but should never contains any include directive inside.
 */
class ISourceChunk {
public:
    virtual std::string get_content() = 0;
    virtual size_t      get_line_count() = 0;

    // Only for include chunk type.
    virtual ShaderSource* get_dependency() = 0;
    virtual void          check_update() = 0;
};

/**
 * \brief Read and parse shader source code. Also handles includes directives and hot reloads.
 */
class ShaderSource final {
public:
    ShaderSource();
    ~ShaderSource() = default;

    /**
     * \brief Get parser result. The resulting string can be compiled.
     */
    [[nodiscard]] std::string get_source_code() const;

    /**
     * \brief When source code have been modified (hot reload or new source file)
     */
    Event_ShaderFileUpdate on_data_changed;

    /**
     * \brief Reload file if any changes have been detected
     */
    void check_update();

    /**
     * \brief Set shader file source path
     * \param source_path 
     */
    void set_source_path(const std::string& source_path);

    /**
     * \brief List of Shader files included in this shader
     */
    [[nodiscard]] std::vector<const ShaderSource*> get_dependencies() const;

    /**
     * \brief Get number of lines of this shader code (including expanded include directives)
     */
    [[nodiscard]] size_t get_line_count() const;

    /**
     * \brief Used to find the original source file containing an error. (within include directives)
     * \param line absolute line in the parsed shader
     * \param local_line line in the found file
     * \return file path
     */
    std::string get_file_at_line(size_t line, size_t& local_line) const;

    [[nodiscard]] const std::string& get_path() const { return source_path; }
    [[nodiscard]] std::string        get_file_name() const;

    struct FileTimeType {
        virtual void* get() = 0;
        template <typename T> T& get() { return *static_cast<T*>(get()); }
    };

private:
    std::vector<std::shared_ptr<ISourceChunk>> content;

    std::string source_path;

    std::shared_ptr<FileTimeType> last_file_update = nullptr;

    void reload_internal();
};

/**
 * \brief A piece of shader code that contains no include directive
 */
class SourceChunkText : public ISourceChunk {
public:
    SourceChunkText(std::string in_text, size_t line_count)
        : text(std::move(in_text)), line_count(line_count) { return; }

    size_t      get_line_count() override { return line_count; }
    std::string get_content() override { return text; }

    ShaderSource* get_dependency() override { return nullptr; }
    void          check_update() override { return void(); }

private:
    size_t            line_count = 0;
    const std::string text;
};

/**
 * \brief Represent an include directive
 */
class SourceChunkDependency : public ISourceChunk {
public:
    std::string   get_content() override { return dependency.get_source_code(); }
    void          check_update() override { dependency.check_update(); }
    ShaderSource* get_dependency() override { return &dependency; }
    size_t        get_line_count() override { return dependency.get_line_count(); }
    ShaderSource  dependency;
};
