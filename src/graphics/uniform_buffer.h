#pragma once
#include <memory>
#include <string>


class UniformBuffer {
public:
    static std::shared_ptr<UniformBuffer> create(const std::string& name) {
        return std::shared_ptr<UniformBuffer>(new UniformBuffer(name));
    }

    virtual ~UniformBuffer();

    void set_data_raw(const void* data_ptr, size_t data_size);

    template <typename Struct_T>
    void set_data(const Struct_T& data) {
        set_data_raw(&data, sizeof(Struct_T));
    }

    [[nodiscard]] uint32_t id() const { return ubo_id; }

    const std::string name;
private:
    uint32_t ubo_id;
    UniformBuffer(const std::string& in_name);
};
