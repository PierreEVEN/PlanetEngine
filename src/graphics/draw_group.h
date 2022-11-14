#pragma once
#include <cstdint>
#include <cstdlib> 
#include <typeinfo>
#include <vector>

class DrawGroupId {
public:
    template <class Type_T> [[nodiscard]] static uint8_t get() {
        static const uint8_t group_id = current_id++;
        return 1 << group_id;
    }

private:
    inline static uint8_t current_id = 1;
};

class DrawGroup {
public:
    DrawGroup() = default;

    template <typename T = void, typename... Ts> static DrawGroup from() {
        DrawGroup group = DrawGroup::from<Ts...>();
        group.draw_groups |= DrawGroupId::get<T>();
        group.names.emplace_back(typeid(T).name());
        return group;
    }

    [[nodiscard]] bool                            contains(const DrawGroup& other) const { return draw_groups & other.draw_groups; }
    [[nodiscard]] const std::vector<const char*>& group_names() { return names; }

private:
    size_t                   draw_groups = 0;
    std::vector<const char*> names;
};

    template <> inline DrawGroup DrawGroup::from<>() { return {}; }

class DrawGroup_View {
};

class DrawGroup_Reflections {
};

class DrawGroup_Shadows {
};

class DrawGroup_WaterMask {
};

class DrawGroup_Translucency {
};