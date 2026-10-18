#include "obj_loader.hpp"

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
    for (const auto &index : obj_shape.mesh.indices) {
      graphics::Vertex vertex;
      for (int i = 0; i < 3; i++) {
        vertex.position[i] = attrib.vertices[3 * index.vertex_index + i];
      }
      vertex.color = {random_color(), random_color(), random_color()};
      shape.vertices.push_back(vertex);
      shape.indices.push_back(shape.indices.size());
    }
    shapes.push_back(shape);
  }
  return shapes;
}
} // namespace loaders
