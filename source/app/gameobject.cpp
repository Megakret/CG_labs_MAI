#include "gameobject.hpp"
#include "graphics/figure.hpp"
#include <memory>

namespace application {
Gameobject::Gameobject(const graphics::Figure::Shape &shape,
                       VkDescriptorPool pool,
                       VkDescriptorSetLayout descriptor_layout)
    : figure(shape, pool, descriptor_layout) {}
graphics::FigureState Gameobject::GetFigureState() {
  return graphics::FigureState{.position = position,
                               .rotation = rotation,
                               .scale = scale,
                               .color = color};
}
GameobjectPtr
GameobjectManager::Instantiate(const graphics::Figure::Shape &shape) {
  std::shared_ptr<Gameobject> ptr =
      std::make_shared<Gameobject>(shape, pool, figure_layout);
  gameobjects.push_back(ptr);
  return ptr;
}
GameobjectPtr
GameobjectManager::Instantiate(const graphics::Figure::Shape &shape,
                               const std::array<float, 3> &position) {
  auto ptr = Instantiate(shape);
  ptr->position = position;
  return ptr;
}
GameobjectPtr GameobjectManager::Instantiate(
    const graphics::Figure::Shape &shape, const std::array<float, 3> &position,
    const std::array<float, 3> &rotation, const std::array<float, 3> &scale) {
  auto ptr = Instantiate(shape, position);
  ptr->rotation = rotation;
  ptr->scale = scale;
  return ptr;
}
GameobjectPtr GameobjectManager::Instantiate(
    const graphics::Figure::Shape &shape, const std::array<float, 3> &position,
    const std::array<float, 3> &rotation, const std::array<float, 3> &scale,
    const std::array<float, 3> &color) {
  auto ptr = Instantiate(shape, position, rotation, scale);
  ptr->color = color;
  return ptr;
}
}; // namespace application
