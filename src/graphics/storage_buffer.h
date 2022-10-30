#pragma once
#include <memory>
#include <string>

class StorageBuffer {
public:
    ~StorageBuffer();

    static std::shared_ptr<StorageBuffer> create(const std::string& name) {
        return std::shared_ptr<StorageBuffer>(new StorageBuffer(name));
    }

    void set_data_raw(const void* data_ptr, size_t data_size);

    template <typename Struct_T>
    void set_data(const Struct_T& data) {
        set_data_raw(&data, sizeof(Struct_T));
    }

    [[nodiscard]] uint32_t id() const { return ssbo_id; }

    const std::string name;

private:
    uint32_t ssbo_id;
    StorageBuffer(const std::string& in_name);
};
