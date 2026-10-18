#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include "animation.hpp"
#include "graphics/figure.hpp"
#include "graphics/material.hpp"

namespace application {
class Gameobject {
public:
  Gameobject(const graphics::Figure::Shape &shape, VkDescriptorPool pool,
             VkDescriptorSetLayout descriptor_layout);
  graphics::FigureState GetFigureState();
  std::array<float, 3> position = {0,0,0};
  std::array<float, 3> rotation = {0,0,0};
  std::array<float, 3> scale = {1,1,1};
  std::array<float, 3> color = {1,1,1};
  graphics::Figure figure;
  graphics::Material material;
  animation::Config animation_config;
  animation::AnimationEngine animation_engine;
};
using GameobjectPtr = std::shared_ptr<Gameobject>;
class GameobjectManager {
public:
  GameobjectManager(VkDescriptorPool pool, VkDescriptorSetLayout figure_layout)
      : pool(pool), figure_layout(figure_layout) {}
  std::vector<GameobjectPtr> gameobjects;
  GameobjectPtr Instantiate(const graphics::Figure::Shape &shape);
  GameobjectPtr Instantiate(const graphics::Figure::Shape &shape,
                            const std::array<float, 3> &position);
  GameobjectPtr Instantiate(const graphics::Figure::Shape &shape,
                            const std::array<float, 3> &position,
                            const std::array<float, 3> &rotation,
                            const std::array<float, 3> &scale);
  GameobjectPtr Instantiate(const graphics::Figure::Shape &shape,
                            const std::array<float, 3> &position,
                            const std::array<float, 3> &rotation,
                            const std::array<float, 3> &scale,
                            const std::array<float, 3> &color);

private:
  VkDescriptorPool pool;
  VkDescriptorSetLayout figure_layout;
};
} // namespace application
