#pragma once
#include "../../graphics_internal.hpp"
#include "glm/fwd.hpp"
#include "graphics.hpp"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace graphics {
    struct FigureState{
        std::array<float, 3> position = {0,0,-20.0f};
        std::array<float, 3> rotation = {0,0,0};
        std::array<float, 3> scale = {1,1,1};
        std::array<float, 3> color = {1,1,1};
    };
class Figure {
public:
  struct Shape {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
  };
  struct UniformData {
    float local_coords_matrix[4][4];
    float transform_projection_matrix[4][4];
    float color[3];
  };

public:
  Figure(const Shape &shape, VkDescriptorPool descriptor_pool,
         VkDescriptorSetLayout descriptor_layout);
  Figure(const Figure &) = delete;
  Figure(Figure &&) = default;
  Figure &operator=(const Figure &) = delete;
  Figure &operator=(Figure &&) = default;
  void Bind(VkCommandBuffer command_buffer);
  void Draw(VkCommandBuffer command_buffer);
  void UpdateFigure(const glm::mat4& projection_matrix,
                    const std::array<float, 3> &position,
                    const std::array<float, 3> &rotation,
                    const std::array<float, 3> &scale,
                    const std::array<float, 3> &color);
  ~Figure();

private:
  VkBuffer vk_vertex_buffer;
  VmaAllocation vk_vertex_buffer_allocation;
  graphics::Vertex *vk_vertex_buffer_memory;

  VkBuffer vk_index_buffer;
  VmaAllocation vk_index_buffer_allocation;
  uint16_t *vk_index_buffer_memory;

  VkBuffer vk_uniform_buffer;
  VmaAllocation vk_uniform_buffer_allocation;
  UniformData *vk_uniform_buffer_memory;
  VkDescriptorSet vk_uniform_descriptor_set;
  size_t index_count = 0;
};
} // namespace graphics
