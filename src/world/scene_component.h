#pragma once
#include <memory>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

class Camera;

class Class
{
public:
	const std::string class_name;
	
	template<typename T>
	Class(T*) : class_name(typeid(T).name())
	{
	}

	template<typename T>
	static Class of() { return Class(reinterpret_cast<T*>(0)); }

	bool operator==(const Class& other) const { return class_name == other.class_name; }
};

class SceneComponent
{
	friend class World;
public:
	SceneComponent(const std::string& in_name) : name(in_name)
	{
	}

	virtual ~SceneComponent() = default;

	virtual void set_local_position(const Eigen::Vector3d& position)
	{
		local_position = position;
		mark_dirty();
	}

	virtual void set_local_rotation(const Eigen::Quaterniond& rotation)
	{
		local_rotation = rotation;
		mark_dirty();
	}

	virtual void set_local_scale(const Eigen::Vector3d& scale)
	{
		local_scale = scale;
		mark_dirty();
	}

	[[nodiscard]] virtual const Eigen::Vector3d& get_local_position() const
	{
		return local_position;
	}

	[[nodiscard]] virtual const Eigen::Quaterniond& get_local_rotation() const
	{
		return local_rotation;
	}

	[[nodiscard]] virtual const Eigen::Vector3d& get_local_scale() const
	{
		return local_scale;
	}

	[[nodiscard]] virtual Eigen::Vector3d get_world_position() const
	{
		if (parent)
			return parent->get_world_transform() * local_position;
		return local_position;
	}

	[[nodiscard]] virtual Eigen::Quaterniond get_world_rotation();

	[[nodiscard]] virtual Eigen::Vector3d get_world_scale() const
	{
		if (parent)
			return parent->get_world_transform() * local_scale;
		return local_scale;
	}

	[[nodiscard]] virtual const Eigen::Affine3d& get_world_transform()
	{
		if (is_dirty)
		{
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

	[[nodiscard]] virtual Eigen::Vector3d world_forward()
	{
		return get_world_transform().rotation() * Eigen::Vector3d(1, 0, 0);
	}

	[[nodiscard]] virtual Eigen::Vector3d world_right()
	{
		return get_world_transform().rotation() * Eigen::Vector3d(0, 1, 0);
	}

	[[nodiscard]] virtual Eigen::Vector3d world_up()
	{
		return get_world_transform().rotation() * Eigen::Vector3d(0, 0, 1);
	}

	void add_child(const std::shared_ptr<SceneComponent>& new_child);

	void detach();

	[[nodiscard]] const std::vector<std::shared_ptr<SceneComponent>>& get_children() const { return children; }

	[[nodiscard]] SceneComponent* get_parent() const { return parent; }

	const std::string name;

	virtual void draw_ui();

	virtual Class get_class() { return Class(this); }

protected:
	virtual void tick(double delta_time)
	{
	}

	virtual void render(Camera& camera)
	{
	}

	void mark_dirty()
	{
		is_dirty = true;
		for (const auto& child : children)
			child->mark_dirty();
	}

private:
	void tick_internal(double delta_time);
	void render_internal(Camera& camera);

	std::vector<std::shared_ptr<SceneComponent>> children;
	SceneComponent* parent = nullptr;

	Eigen::Vector3d local_position = Eigen::Vector3d::Zero();
	Eigen::Quaterniond local_rotation = Eigen::Quaterniond::Identity();
	Eigen::Vector3d local_scale = Eigen::Vector3d::Ones();

	bool is_dirty = true;
	Eigen::Affine3d world_transform;
};
