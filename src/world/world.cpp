#include "world.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <thread>
#include "graphics/camera.h"
#include "utils/profiler.h"

struct WorldDataStructure
{
	alignas(16) Eigen::Matrix4f proj_matrix;
	alignas(16) Eigen::Matrix4f view_matrix;
	alignas(16) Eigen::Matrix4f vp_matrix;
	alignas(16) Eigen::Matrix4f proj_matrix_inv;
	alignas(16) Eigen::Matrix4f view_matrix_inv;
	alignas(16) Eigen::Matrix4f vp_matrix_inv;
	alignas(16) float world_time;
	alignas(16) Eigen::Vector3f camera_pos;
	alignas(16) Eigen::Vector3f camera_forward;
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

	{
		const double required_delta_s = 1.0 / framerate_limit;
		STAT_DURATION("Framerate limiter");
		do
		{
			delta_seconds = std::min(glfwGetTime() - last_time, 1.0);
			if (framerate_limit > 1)
				std::this_thread::sleep_for(
					std::chrono::microseconds(
						static_cast<size_t>(std::max(0.0, required_delta_s - delta_seconds) * 1000000)));
		}
		while (framerate_limit > 1 && delta_seconds < required_delta_s);
	}
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
		.view_matrix_inv = view_matrix.cast<float>().inverse(),
		.vp_matrix_inv = pv_matrix.cast<float>().inverse(),
		.world_time = static_cast<float>(glfwGetTime()),
		.camera_pos = camera->get_world_position().cast<float>(),
		.camera_forward = camera->world_forward().cast<float>(),
	};
	glBindBuffer(GL_UNIFORM_BUFFER, world_uniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(WorldDataStructure), &world_data, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	root_component->tick_internal(delta_seconds);
}

void World::render_world() const
{
	STAT_DURATION("World render");
	root_component->render_internal(*camera);
}

std::shared_ptr<Camera> World::get_camera() const
{
	return camera;
}
