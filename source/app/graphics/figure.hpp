#pragma once

#include "descriptors.hpp"
#include "glm/fwd.hpp"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "graphics.hpp"
#include "material.hpp"
#include "uniform_buffer.hpp"

namespace graphics {
const uint32_t FIGURE_SET_ID = 1;
const uint32_t FIGURE_BINDING = 0;
struct FigureState {
  std::array<float, 3> position = {0, 0, -20.0f};
  std::array<float, 3> rotation = {0, 0, 0};
  std::array<float, 3> scale = {1, 1, 1};
  std::array<float, 3> color = {1, 1, 1};
};
class Figure {
public:
  struct Shape {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
  };
  struct UniformData {
    float model[4][4];
    float color[3];
    float a;
    graphics::Material material;
  };

public:
  Figure(const Shape &shape, graphics::DescriptorSet& descriptor_set);
  Figure(const Figure &) = delete;
  Figure(Figure &&) noexcept;
  Figure &operator=(const Figure &) = delete;
  Figure &operator=(Figure &&) noexcept;
  void Bind(VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout);
  void Draw(VkCommandBuffer command_buffer);
  void UpdateFigure(const std::array<float, 3> &position,
                    const std::array<float, 3> &rotation,
                    const std::array<float, 3> &scale,
                    const std::array<float, 3> &color);
  void UpdateMaterial(const graphics::Material &material);
  ~Figure();

private:
  VkBuffer vk_vertex_buffer;
  VmaAllocation vk_vertex_buffer_allocation;
  graphics::Vertex *vk_vertex_buffer_memory;

  VkBuffer vk_index_buffer;
  VmaAllocation vk_index_buffer_allocation;
  uint16_t *vk_index_buffer_memory;

  graphics::DescriptorSet descriptor_set;
  UniformBuffer<UniformData> uniform_buffer;
  size_t index_count = 0;
};
} // namespace graphics
