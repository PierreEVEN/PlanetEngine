#pragma once
#include "graphics/draw_group.h"

#include <memory>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

class RenderPass;
class Camera;

enum class TickGroup {
    PrePhysic,
    Physic,
    PostPhysic,
};


/**
 * \brief Represent the class of an object. Helper used to know the type of Scene component
 */
class Class {
public:
    const std::string class_name;

    template <typename T>
    Class(T*)
        : class_name(typeid(T).name()) {
    }

    template <typename T>
    static Class of() { return Class(reinterpret_cast<T*>(0)); }

    bool operator==(const Class& other) const { return class_name == other.class_name; }
};

class SceneComponent {
    friend class World;
public:
    SceneComponent(std::string in_name)
        : name(std::move(in_name)) {
    }

    virtual ~SceneComponent() = default;

    /**
     * \brief Set component's location in local space
     */
    virtual void set_local_position(const Eigen::Vector3d& position) {
        local_position = position;
        mark_dirty();
    }

    /**
     * \brief Set component's rotation in local space using quaternions
     */
    virtual void set_local_rotation(const Eigen::Quaterniond& rotation) {
        local_rotation = rotation;
        mark_dirty();
    }

    /**
     * \brief Set component's scale factor in local space
     * \param scale 
     */
    virtual void set_local_scale(const Eigen::Vector3d& scale) {
        local_scale = scale;
        mark_dirty();
    }

    /**
     * \brief Get component's position in local space
     */
    [[nodiscard]] virtual const Eigen::Vector3d& get_local_position() const {
        return local_position;
    }

    /**
     * \brief Get component's rotation in local space using quaternions
     * \return 
     */
    [[nodiscard]] virtual const Eigen::Quaterniond& get_local_rotation() const {
        return local_rotation;
    }

    /**
     * \brief Get component's scale factor in local space
     * \return 
     */
    [[nodiscard]] virtual const Eigen::Vector3d& get_local_scale() const {
        return local_scale;
    }

    /**
     * \brief Get component's position in world space.
     */
    [[nodiscard]] virtual Eigen::Vector3d get_world_position() const {
        if (parent)
            return parent->get_world_transform() * local_position;
        return local_position;
    }

    /**
     * \brief Get component's rotation in world space
     */
    [[nodiscard]] virtual Eigen::Quaterniond get_world_rotation();

    /**
     * \brief Get component's scale factor in world space
     */
    [[nodiscard]] virtual Eigen::Vector3d get_world_scale() const {
        if (parent)
            return parent->get_world_rotation() * local_scale;
        return local_scale;
    }

    /**
     * \brief Compute component's transformation matrix in world space
     */
    [[nodiscard]] virtual const Eigen::Affine3d& get_world_transform() {
        if (is_dirty) {
            world_transform = Eigen::Affine3d::Identity();
            world_transform.translate(local_position);
            world_transform.rotate(local_rotation);
            world_transform.scale(local_scale); //@TODO : fix scale
            if (parent)
                world_transform = parent->get_world_transform() * world_transform;

            is_dirty = false;
        }
        return world_transform;
    }

    /**
     * \brief Vector pointing toward forward direction of the component. (1, 0, 0)
     */
    [[nodiscard]] virtual Eigen::Vector3d world_forward() {
        return get_world_transform().rotation() * Eigen::Vector3d(1, 0, 0);
    }

    /**
     * \brief Vector pointing right in component's space. We use a right-handed space. (0, 1, 0)
     */
    [[nodiscard]] virtual Eigen::Vector3d world_right() {
        return get_world_transform().rotation() * Eigen::Vector3d(0, 1, 0);
    }

    /**
     * \brief Vector pointing up in component's local space. We use a z-up space. (0, 0, 1)
     */
    [[nodiscard]] virtual Eigen::Vector3d world_up() {
        return get_world_transform().rotation() * Eigen::Vector3d(0, 0, 1);
    }

    /**
     * \brief Attach given component to this one. The world transformation could be different.
     * \param new_child 
     */
    void add_child(const std::shared_ptr<SceneComponent>& new_child);

    /**
     * \brief Make this component orphan. Detached component are not visible or updated.
     */
    void detach();

    /**
     * \brief Get all children directly attached to this component.
     */
    [[nodiscard]] const std::vector<std::shared_ptr<SceneComponent>>& get_children() const { return children; }

    /**
     * \brief Get direct parent for this component.
     */
    [[nodiscard]] SceneComponent* get_parent() const { return parent; }

    /**
     * \brief Component display name.
     */
    const std::string name;

    /**
     * \brief Called when the object is selected in the world outliner.
     */
    virtual void draw_ui();

    /**
     * \brief Class of the component. Implement this method for every child class.
     */
    virtual Class get_class() { return {this}; }

    /**
     * \brief The tick group define the priority of tick (first groups are called first)
     */
    void set_tick_group(TickGroup new_group) { tick_group = new_group; }

    void set_draw_group(DrawGroup new_group) { draw_group = new_group; }

    std::vector<std::shared_ptr<SceneComponent>> get_all_components_of_class(const Class& component_class) const;

protected:
    /**
     * \brief Called once per frame. Used for gameplay purposes.
     * \param delta_time elapsed time between frames in seconds
     */
    virtual void tick(double delta_time) {
    }

    /**
     * \brief Called each time this object have to be rendered
     * \param camera Desired point of view
     */
    virtual void render(Camera& camera, const DrawGroup& draw_group, const std::shared_ptr<RenderPass>& render_pass) {
    }

    /**
     * \brief Notify world transform have been changed and should be recomputed.
     */
    void mark_dirty() {
        is_dirty = true;
        for (const auto& child : children)
            child->mark_dirty();
    }

    DrawGroup draw_group = DrawGroup::from<DrawGroup_View>();

private:
    void tick_internal(double delta_time, TickGroup new_group);
    void render_internal(Camera& camera, const DrawGroup& draw_group, const std::shared_ptr<RenderPass>& render_pass);

    std::vector<std::shared_ptr<SceneComponent>> children;
    SceneComponent*                              parent = nullptr;

    Eigen::Vector3d    local_position = Eigen::Vector3d::Zero();
    Eigen::Quaterniond local_rotation = Eigen::Quaterniond::Identity();
    Eigen::Vector3d    local_scale    = Eigen::Vector3d::Ones();

    bool            is_dirty = true;
    Eigen::Affine3d world_transform;
    TickGroup       tick_group = TickGroup::Physic;
};
