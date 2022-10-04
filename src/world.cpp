#include "world.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "camera.h"
#include "engine/engine.h"
#include "utils/profiler.h"

struct WorldDataStructure
{
	alignas(16) Eigen::Matrix4f proj_matrix;
	alignas(16) Eigen::Matrix4f view_matrix;
	alignas(16) Eigen::Matrix4f vp_matrix;
	alignas(16) Eigen::Matrix4f proj_matrix_inv;
	alignas(16) Eigen::Matrix4f view_matrix_inv;
	alignas(16) Eigen::Matrix4f vp_matrix_inv;
	alignas(16) Eigen::Vector3f camera_pos;
	alignas(16) Eigen::Vector3f camera_forward;
	alignas(16) float world_time;
};

World::World() : camera(std::make_shared<Camera>()), root_component(std::make_unique<SceneComponent>("root"))
{
	glGenBuffers(1, &world_uniform);
	glBindBuffer(GL_UNIFORM_BUFFER, world_uniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(WorldDataStructure), nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, world_uniform, 0, sizeof(WorldDataStructure));
	root_component->add_child(camera);
}

World::~World()
{
	glDeleteBuffers(1, &world_uniform);
}

void World::tick_world()
{
	STAT_DURATION("World tick");
	delta_seconds = std::min(glfwGetTime() - last_time, 1 / 15.0);
	last_time = glfwGetTime();

	// Update world data
	const auto proj_matrix = camera->reversed_z_projection_matrix();
	const auto view_matrix = camera->view_matrix();
	const auto pv_matrix = proj_matrix * view_matrix;
	const WorldDataStructure world_data = {
		.proj_matrix = proj_matrix.cast<float>(),
		.view_matrix = view_matrix.cast<float>(),
		.vp_matrix = pv_matrix.cast<float>(),
		.proj_matrix_inv = proj_matrix.cast<float>().inverse(),
		.view_matrix_inv  = view_matrix.cast<float>().inverse(),
		.vp_matrix_inv = pv_matrix.cast<float>().inverse(),
		.camera_pos = camera->get_world_position().cast<float>(),
		.camera_forward = camera->world_forward().cast<float>(),
		.world_time = static_cast<float>(glfwGetTime())
	};
	glBindBuffer(GL_UNIFORM_BUFFER, world_uniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(world_data), &world_data, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	root_component->tick_internal(delta_seconds);
}

void World::render_world() const
{
	STAT_DURATION("World render");
	root_component->render_internal();
}
