#include "primitives.h"
#include "graphics/mesh.h"

#include <iostream>

namespace primitives {

std::shared_ptr<Mesh> cube(float size) {
    auto mesh = Mesh::create("cube");

    float V = size;
    float v = -size;

    mesh->set_positions(std::vector<Eigen::Vector3f>{
                            {v, v, v}, {V, v, v}, {V, V, v}, {v, V, v},
                            {v, v, V}, {V, v, V}, {V, V, V}, {v, V, V},
                            {v, v, V}, {v, v, v}, {v, V, v}, {v, V, V},
                            {V, v, V}, {V, v, v}, {V, V, v}, {V, V, V},
                            {v, v, V}, {V, v, V}, {V, v, v}, {v, v, v},
                            {v, V, V}, {V, V, V}, {V, V, v}, {v, V, v}}, 0, true);

    mesh->set_colors(std::vector<Eigen::Vector3f>{
                         {1, 1, 0}, {1, 1, 0}, {1, 1, 0}, {1, 1, 0},
                         {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
                         {0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1},
                         {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0},
                         {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
                         {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}}, 3, true);

    mesh->set_normals(std::vector<Eigen::Vector3f>{
                          {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
                          {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
                          {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0},
                          {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0},
                          {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0},
                          {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}}, 1, true);

    mesh->set_texture_coordinates(std::vector<Eigen::Vector2f>{
                                      {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                      {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                      {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                      {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                      {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                      {0, 0}, {1, 0}, {1, 1}, {0, 1}}, 2, true);

    mesh->set_indices(std::vector<uint32_t>{0, 3, 2, 0, 2, 1, 4, 5, 6, 4, 6, 7, 8, 11, 10, 8, 10, 9, 12, 13, 14, 12, 14, 15, 16, 19, 18, 16, 18, 17, 20, 21, 22, 20, 22, 23}, true);

    mesh->rebuild_mesh_data();

    return mesh;
}

std::shared_ptr<Mesh> grid_plane(int res_x, int res_y, float cell_size) {
    auto mesh = Mesh::create("grid");

    std::vector<Eigen::Vector3f> positions(res_x * res_y);
    std::vector<Eigen::Vector3f> colors(res_x * res_y);
    std::vector<Eigen::Vector3f> normals(res_x * res_y);
    std::vector<Eigen::Vector2f> tc(res_x * res_y);
    for (int x = 0; x < res_x; ++x) {
        for (int y = 0; y < res_y; ++y) {
            positions[x + y * res_x] = {x * cell_size, y * cell_size, 0};
            colors[x + y * res_x]    = {x * cell_size, y * cell_size, 0};
            normals[x + y * res_x]   = {0, 0, 1};
            tc[x + y * res_x]        = {x / res_x, y / res_y};
        }
    }

    mesh->set_positions(positions, 0, true);
    mesh->set_colors(colors, 3, true);
    mesh->set_normals(normals, 1, true);
    mesh->set_texture_coordinates(tc, 2, true);

    std::vector<uint32_t> ids((res_x - 1) * (res_y - 1) * 6);
    for (int x = 0; x < res_x - 1; ++x) {
        for (int y = 0; y < res_y - 1; ++y) {
            const int start = (x + y * (res_x - 1)) * 6;
            ids[start]      = x + y * res_x;
            ids[start + 1]  = x + 1 + y * res_x;
            ids[start + 2]  = x + (y + 1) * res_x;
            ids[start + 3]  = x + 1 + y * res_x;
            ids[start + 4]  = x + 1 + (y + 1) * res_x;
            ids[start + 5]  = x + (y + 1) * res_x;
        }
    }

    mesh->set_indices(ids, true);

    mesh->rebuild_mesh_data();

    return mesh;
}
}
