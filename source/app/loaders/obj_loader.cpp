#include "obj_loader.hpp"

#include <cstdint>
#include <format>
#include <random>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace {
float random_color() {
  static thread_local std::mt19937 engine{std::random_device{}()};
  static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  return dist(engine);
}
} // namespace
namespace loaders {
std::vector<graphics::Figure::Shape>
load_models_obj(std::filesystem::path model_path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> obj_shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &obj_shapes, &materials, &err,
                        model_path.c_str())) {
    throw std::runtime_error(err);
  }
  std::vector<graphics::Figure::Shape> shapes;
  for (size_t i = 0; i < obj_shapes.size(); i++) {
    auto &obj_shape = obj_shapes[i];
    graphics::Figure::Shape shape;
    using VertexKey = std::tuple<uint16_t, uint16_t>;
    std::map<VertexKey, uint16_t> obj_to_engine_indices;
    for (const auto &index : obj_shape.mesh.indices) {
      uint16_t vertex_index = index.vertex_index;
      uint16_t normal_index = index.normal_index;
      auto [it, inserted] = obj_to_engine_indices.insert(
          {{vertex_index, normal_index},
           static_cast<uint16_t>(shape.vertices.size())});
      graphics::Vertex vertex;
      for (int i = 0; i < 3; i++) {
        vertex.position[i] = attrib.vertices[3 * index.vertex_index + i];
      }
      if (index.normal_index >= 0) {
        for (int i = 0; i < 3; i++) {
          vertex.normal[i] = attrib.normals[3 * index.normal_index + i];
        }
      }
      vertex.color = {random_color(), random_color(), random_color()};
      if (inserted) {
        shape.vertices.push_back(vertex);
      }
      shape.indices.push_back(it->second);
    }
    shapes.push_back(shape);
    std::cout << std::format("LOADED. Vertices: {}. Indices: {}\n",
                             shape.vertices.size(), shape.indices.size());
  }
  return shapes;
}
} // namespace loaders
