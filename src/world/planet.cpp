#include <GL/gl3w.h>

#include "graphics/camera.h"
#include "planet.h"

#include <imgui.h>

#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/compute_shader.h"
#include "graphics/mesh.h"
#include "graphics/material.h"
#include "graphics/storage_buffer.h"
#include "graphics/texture_image.h"
#include "ui/widgets.h"
#include "utils/game_settings.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"

static std::shared_ptr<Material> planet_material = nullptr;
static std::shared_ptr<Texture2D> grass = nullptr;
static std::shared_ptr<Texture2D> grass_normal = nullptr;
static std::shared_ptr<Texture2D> rock = nullptr;
static std::shared_ptr<Texture2D> rock_normal = nullptr;
static std::shared_ptr<Texture2D> sand = nullptr;
static std::shared_ptr<Texture2D> sand_normal = nullptr;
static std::shared_ptr<ComputeShader> compute_positions = nullptr;
static std::shared_ptr<ComputeShader> compute_normals = nullptr;
static std::shared_ptr<ComputeShader> fix_seams = nullptr;


std::shared_ptr<Material> Planet::get_landscape_material()
{
	if (planet_material)
		return planet_material;
	STAT_ACTION("Create planet resources");
	planet_material = Material::create("planet material");
	planet_material->load_from_source("resources/shaders/planet_material.vs",
	                                  "resources/shaders/planet_material.fs");

	grass = Texture2D::create("terrain grass");
	grass->from_file("resources/textures/terrain/wispy-grass-meadow_albedo.png");

	grass_normal = Texture2D::create("terrain grass normal");
	grass_normal->from_file("resources/textures/terrain/wispy-grass-meadow_normal-dx.png");

	rock = Texture2D::create("terrain rock");
	rock->from_file("resources/textures/terrain/pine_forest_ground1_albedo.png");

	rock_normal = Texture2D::create("terrain rock normal");
	rock_normal->from_file("resources/textures/terrain/pine_forest_ground1_Normal-dx.png");

	sand = Texture2D::create("terrain sand");
	sand->from_file("resources/textures/terrain/wavy-sand_albedo.png");

	sand_normal = Texture2D::create("terrain sand normal");
	sand_normal->from_file("resources/textures/terrain/wavy-sand_normal-dx.png");

	compute_positions = ComputeShader::create("Planet compute position");
	compute_positions->load_from_source("resources/shaders/compute/planet_compute_position.cs");

	fix_seams = ComputeShader::create("Planet Fix Seams");
	fix_seams->load_from_source("resources/shaders/compute/planet_fix_seams.cs");

	compute_normals = ComputeShader::create("Planet compute normals");
	compute_normals->load_from_source("resources/shaders/compute/planet_compute_normals.cs");

	return planet_material;
}


static double snap(double value, double delta) { return round(value / delta) * delta; }

Planet::Planet(const std::string& name) : SceneComponent(name), world(Engine::get().get_world())
{
	root = std::make_shared<PlanetRegion>(*this, world, 16, 0);
	dirty = true;

	get_landscape_material();

	compute_positions->on_reload.add_object(root.get(), &PlanetRegion::force_rebuild_maps);
	fix_seams->on_reload.add_object(root.get(), &PlanetRegion::force_rebuild_maps);
	compute_normals->on_reload.add_object(root.get(), &PlanetRegion::force_rebuild_maps);
}

void Planet::draw_ui()
{
	SceneComponent::draw_ui();
	ImGui::SliderInt("num LODs : ", &num_lods, 1, 40);
	ImGui::DragFloat("radius : ", &radius, 10);
	if (ImGui::SliderInt("cell number", &cell_count, 1, 40) ||
		ImGui::SliderFloat("cell_width : ", &cell_width, 0.05f, 10))
		dirty = true;
	ImGui::Separator();
	ImGui::Checkbox("Double sided", &double_sided);
	ImGui::Checkbox("Freeze Camera", &freeze_camera);
	ImGui::Checkbox("Freeze Updates", &freeze_updates);
	ImGui::DragFloat4("debug vector", debug_vector.data());

	Eigen::Quaterniond rot = Eigen::Quaterniond(world_orientation.rotation());

	if (ui::rotation_edit(rot, "camera rotation"))
	{
		world_orientation = Eigen::Affine3d::Identity();
		world_orientation.rotate(rot);
	}
}

static void generate_rectangle_area(std::vector<uint32_t>& indices, std::vector<Eigen::Vector3f>& positions,
                                    int32_t x_min, int32_t x_max, int32_t z_min, int32_t z_max,
                                    int32_t cell_min, int32_t cell_max)
{
	// Requirement for Y val
	const float distance_max = static_cast<float>(cell_max);
	float min_global = static_cast<float>(cell_min);

	// Handle LOD 0 case
	if (min_global == distance_max)
		min_global = 0;

	const auto current_index_offset = static_cast<uint32_t>(positions.size());
	for (int32_t z = z_min; z <= z_max; ++z)
		for (int32_t x = x_min; x <= x_max; ++x)
		{
			// The Y val is used to store the progression from previous LOD to the next one
			const float Linf_distance = static_cast<float>(std::max(std::abs(x), std::abs(z))); // Tchebychev distance
			const float y_val_weight = (Linf_distance - min_global) / (distance_max - min_global);
			const bool x_aligned = std::abs(x) > std::abs(z);
			const int mask = std::abs(x) % 2 == 0 && !x_aligned || std::abs(z) % 2 == 0 && x_aligned;
			positions.emplace_back(Eigen::Vector3f(static_cast<float>(x), mask * y_val_weight, static_cast<float>(z)));
		}

	const uint32_t x_width = std::abs(x_max - x_min);
	const uint32_t z_width = std::abs(z_max - z_min);
	for (uint32_t z = 0; z < z_width; ++z)
		for (uint32_t x = 0; x < x_width; ++x)
		{
			uint32_t base_index = x + z * (x_width + 1) + current_index_offset;
			if (positions[base_index].x() * positions[base_index].z() > 0)
			{
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + x_width + 2);
				indices.emplace_back(base_index + x_width + 1);
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 2);
			}
			else
			{
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 1);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 2);
				indices.emplace_back(base_index + x_width + 1);
			}
		}
}

void Planet::regenerate()
{
	GL_CHECK_ERROR();
	STAT_ACTION("Generate planet mesh : [" + name + "]");
	STAT_FRAME("regenerate planet mesh");
	std::vector<Eigen::Vector3f> positions_root;
	std::vector<uint32_t> indices_root;

	{
		STAT_ACTION("regenerate root mesh");
		generate_rectangle_area(indices_root, positions_root,
			-cell_count * 2 - 1,
			cell_count * 2 + 1,
			-cell_count * 2 - 1,
			cell_count * 2 + 1,
			0, cell_count * 2);

		root_mesh = Mesh::create("planet root mesh");
		root_mesh->set_positions(positions_root, 0, true);
		root_mesh->set_indices(indices_root);
	}

	GL_CHECK_ERROR();

	std::vector<Eigen::Vector3f> positions_child;
	std::vector<uint32_t> indices_child;

	{
		STAT_ACTION("Generate planet mesh vertices : [" + name + "]");
		// TOP side (larger)
		generate_rectangle_area(indices_child, positions_child,
			cell_count,
			cell_count * 2 + 1,
			-cell_count - 1,
			cell_count * 2 + 1,
			cell_count, cell_count * 2 + 1);

		// RIGHT side (larger)
		generate_rectangle_area(indices_child, positions_child,
			-cell_count * 2 - 1,
			cell_count,
			cell_count,
			cell_count * 2 + 1,
			cell_count, cell_count * 2 + 1);

		// BOTTOM side
		generate_rectangle_area(indices_child, positions_child,
			-cell_count * 2 - 1,
			-cell_count - 1,
			-cell_count * 2 - 1,
			cell_count,
			cell_count + 1, cell_count * 2 + 1);

		// LEFT side
		generate_rectangle_area(indices_child, positions_child,
			-cell_count - 1,
			cell_count * 2 + 1,
			-cell_count * 2 - 1,
			-cell_count - 1,
			cell_count + 1, cell_count * 2 + 1);
	}
	child_mesh = Mesh::create("planet child mesh");
	child_mesh->set_positions(positions_child, 0, true);
	child_mesh->set_indices(indices_child);

	GL_CHECK_ERROR();

	STAT_ACTION("regenerate planet children chunk : [" + name + "] ");
	root->regenerate(cell_count);
	GL_CHECK_ERROR();
	dirty = false;
}

void Planet::tick(double delta_time)
{
	STAT_FRAME("Planet_Tick");
	SceneComponent::tick(delta_time);

	if (dirty)
	{
		regenerate();
	}

	{
		STAT_FRAME("compute planet global transform");

		if (!freeze_camera)
		{
			// Get camera direction from planet center
			const auto camera_direction = get_world_rotation().inverse() * (Engine::get().get_world().get_camera()->
				get_world_position() - get_world_position()).normalized();

			// Compute global rotation snapping step
			const double max_cell_radian_step = cell_width * std::pow(2, num_lods) / (radius * 2);

			// Compute global planet rotation (orient planet mesh toward camera)
			const auto pitch = asin(camera_direction.z());
			const auto yaw = atan2(camera_direction.y(), camera_direction.x());
			local_orientation = Eigen::Affine3d(
				Eigen::AngleAxisd(snap(yaw, max_cell_radian_step), Eigen::Vector3d::UnitZ()) *
				Eigen::AngleAxisd(snap(-pitch, max_cell_radian_step), Eigen::Vector3d::UnitY())
			);
			world_orientation = get_world_rotation() * local_orientation;
		}

		planet_inverse_rotation = world_orientation.inverse();
		// Compute global planet transformation (ensure ground is always close to origin)
		planet_global_transform = Eigen::Affine3d::Identity();
		planet_global_transform.translate(
			get_world_position() - Engine::get().get_world().get_camera()->get_world_position());
		planet_global_transform = planet_global_transform * world_orientation;
		planet_global_transform.translate(Eigen::Vector3d(radius, 0, 0));
	}

	const double camera_distance_to_ground = (Engine::get().get_world().get_camera()->
	                                                        get_world_position() -
		get_world_position()).norm() - static_cast<double>(radius);

	const double normalized_distance = std::max(1.0, camera_distance_to_ground / (cell_width * (cell_count * 4 + 2)));
	const int min_lod = std::min(num_lods - 1, static_cast<int>(std::log2(normalized_distance)));
	const int max_lod = num_lods;
	const float initial_cell_width = cell_width * static_cast<float>(std::pow(2, min_lod));

	root->tick(delta_time, max_lod - min_lod, initial_cell_width);
}

void Planet::render(Camera& camera)
{
	STAT_FRAME("Render Planet");
	SceneComponent::render(camera);
	root->render(camera);
}

PlanetRegion::PlanetRegion(Planet& in_parent, const World& in_world, uint32_t in_lod_level,
                           uint32_t in_my_level) :
	world(in_world), num_lods(in_lod_level), current_lod(in_my_level), planet(in_parent)
{
}

void PlanetRegion::regenerate(int32_t in_cell_number)
{
	cell_number = in_cell_number;
	{
		const int map_size = cell_number * 4 + 5;
		if (!chunk_height_map || chunk_height_map->width() != map_size)
		{
			GL_CHECK_ERROR();
			chunk_height_map = Texture2D::create("heightmap_LOD_" + std::to_string(current_lod),
				{
					.wrapping = TextureWrapping::ClampToEdge,
					.filtering_mag = TextureMagFilter::Nearest,
					.filtering_min = TextureMinFilter::Nearest
				});
			chunk_height_map->set_data(map_size, map_size, GL_RG32F);
			GL_CHECK_ERROR();
		}
		if (!chunk_normal_map || chunk_normal_map->width() != map_size)
		{
			GL_CHECK_ERROR();
			chunk_normal_map = Texture2D::create("normal_LOD_" + std::to_string(current_lod), {
													 .wrapping = TextureWrapping::ClampToEdge,
													 .filtering_mag = TextureMagFilter::Nearest,
													 .filtering_min = TextureMinFilter::Nearest
				});
			chunk_normal_map->set_data(map_size, map_size, GL_RG16F);
			GL_CHECK_ERROR();
		}

		rebuild_maps();
	}
	if (child)
		child->regenerate(cell_number);
}


void PlanetRegion::tick(double delta_time, int in_num_lods, double in_width)
{
	cell_size = in_width; // in_width;
	// Create or destroy children
	num_lods = in_num_lods;
	if (!child && current_lod + 1 < num_lods)
	{
		child = std::make_shared<PlanetRegion>(planet, world, num_lods, current_lod + 1);
		child->regenerate(cell_number);
	}
	if (child && current_lod >= num_lods - 1)
		child = nullptr;

	if (child)
		child->tick(delta_time, num_lods, cell_size * 2);

	STAT_FRAME("Planet Tick LOD :" + std::to_string(current_lod));

	// Compute camera position in local space
	const Eigen::Vector3d camera_local_position = planet.planet_inverse_rotation * (world.get_camera()->
		get_world_position() - planet.get_world_position());


	const Eigen::Vector3d temp = Eigen::Vector3d(
		0,
		Eigen::Vector3d(camera_local_position.x(), camera_local_position.y(), 0).normalized().y(),
		Eigen::Vector3d(camera_local_position.x(), camera_local_position.y(), camera_local_position.z()).normalized().
		z()
	);


	// Convert linear position to position on sphere // @TODO Minor fix required here
	Eigen::Vector3d local_location = Eigen::Vector3d(
		0,
		asin(std::clamp(temp.y(), -1.0, 1.0)),
		asin(std::clamp(temp.z(), -1.0, 1.0))) * planet.radius;

	const double snapping = cell_size * 2;
	chunk_position = Eigen::Vector3d(
		std::round(local_location.y() / snapping + 0.5) - 0.5,
		0,
		std::round(local_location.z() / snapping + 0.5) - 0.5) * snapping;


	lod_local_transform = Eigen::Affine3d::Identity();
	lod_local_transform.translate(chunk_position);
	lod_local_transform.scale(cell_size);

	Eigen::AngleAxisd rotation = Eigen::AngleAxisd::Identity();

	if (current_lod != 0)
	{
		if (local_location.y() >= chunk_position.x() && local_location.z() < chunk_position.z())
			rotation = Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitY());
		else if (local_location.y() < chunk_position.x() && local_location.z() >= chunk_position.z())
			rotation = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitY());
		else if (local_location.y() >= chunk_position.x() && local_location.z() >= chunk_position.z())
			rotation = Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitY());
		lod_local_transform.rotate(rotation);
	}

	rebuild_maps();
}

void PlanetRegion::render(Camera& camera)
{
	if (child)
		child->render(camera);
	GL_CHECK_ERROR();
	STAT_FRAME("Render planet lod " + std::to_string(current_lod));
	// Set uniforms
	GL_CHECK_ERROR();
	if (Planet::get_landscape_material()->bind())
	{
		glUniform1f(Planet::get_landscape_material()->binding("radius"), planet.radius);
		glUniform1f(Planet::get_landscape_material()->binding("grid_cell_count"),
		            static_cast<float>(planet.cell_count));
		glUniform4fv(Planet::get_landscape_material()->binding("debug_vector"), 1, planet.debug_vector.data());
		glUniformMatrix4fv(Planet::get_landscape_material()->binding("lod_local_transform"), 1, false,
		                   lod_local_transform.cast<float>().matrix().data());
		glUniformMatrix4fv(Planet::get_landscape_material()->binding("planet_world_orientation"), 1, false,
			planet.local_orientation.cast<float>().matrix().data());
		Planet::get_landscape_material()->set_model_transform(planet.planet_global_transform);

		// Bind maps
		Planet::get_landscape_material()->bind_texture(chunk_height_map, "height_map");
		Planet::get_landscape_material()->bind_texture(chunk_normal_map, "normal_map");

		// Bind textures
		Planet::get_landscape_material()->bind_texture(grass, "grass_color");
		Planet::get_landscape_material()->bind_texture(rock, "rock_color");
		Planet::get_landscape_material()->bind_texture(sand, "sand_color");
		Planet::get_landscape_material()->bind_texture(grass_normal, "grass_normal");
		Planet::get_landscape_material()->bind_texture(rock_normal, "rock_normal");
		Planet::get_landscape_material()->bind_texture(sand_normal, "sand_normal");

		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GameSettings::get().wireframe ? GL_LINE : GL_FILL);
		if (current_lod == 0)
			planet.root_mesh->draw();
		else
			planet.child_mesh->draw();
		if (planet.double_sided)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glFrontFace(GL_CW);
			if (current_lod == 0)
				planet.root_mesh->draw();
			else
				planet.child_mesh->draw();
			glFrontFace(GL_CCW);
		}
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void PlanetRegion::rebuild_maps()
{
	if (planet.freeze_updates && !force_rebuild)
		return;

	GL_CHECK_ERROR();
	STAT_FRAME("rebuild landscape map");

	const LandscapeChunkData chunk_data{
		.Chunk_LocalTransform = lod_local_transform.cast<float>().matrix(),
		.Chunk_PlanetModel = (planet.get_world_transform().inverse() * planet.planet_global_transform).cast<float>().
		matrix(),
		.Chunk_LocalOrientation = planet.local_orientation.cast<float>().matrix(),
		.Chunk_PlanetRadius = planet.radius,
		.Chunk_CellWidth = static_cast<float>(cell_size),
		.Chunk_CellCount = cell_number,
		.Chunk_CurrentLOD = static_cast<int32_t>(current_lod)
	};

	if (chunk_data == last_chunk_data && !force_rebuild)
		return;

	force_rebuild = false;

	const auto ssbo = StorageBuffer::create("PlanetChunkData");
	ssbo->set_data(chunk_data);
	last_chunk_data = chunk_data;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo->id());

	planet.get_landscape_material();

	// Compute heightmaps
	compute_positions->bind();
	compute_positions->bind_texture(chunk_height_map, BindingMode::Out, 0);
	compute_positions->execute(chunk_height_map->width(), chunk_height_map->height(), 1);

	// Fix seams
	fix_seams->bind();
	fix_seams->bind_texture(chunk_height_map, BindingMode::InOut, 0);
	fix_seams->execute(chunk_height_map->width(), chunk_height_map->height(), 1);

	// Compute normals
	compute_normals->bind();
	compute_normals->bind_texture(chunk_height_map, BindingMode::In, 0);
	compute_normals->bind_texture(chunk_normal_map, BindingMode::Out, 1);
	compute_normals->execute(chunk_height_map->width(), chunk_height_map->height(), 1);

	GL_CHECK_ERROR();
}

void PlanetRegion::force_rebuild_maps()
{
	force_rebuild = true;
	if (child)
		child->force_rebuild_maps();
}

bool PlanetRegion::LandscapeChunkData::operator==(const LandscapeChunkData& other) const
{
	return Chunk_LocalTransform == other.Chunk_LocalTransform &&
		Chunk_LocalOrientation == other.Chunk_LocalOrientation &&
		Chunk_PlanetRadius == other.Chunk_PlanetRadius &&
		Chunk_CellWidth == other.Chunk_CellWidth &&
		Chunk_CellCount == other.Chunk_CellCount &&
		Chunk_CurrentLOD == other.Chunk_CurrentLOD;
}
