#include "figure.hpp"

#include <iostream>
#include <string.h>

#include "descriptors.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <vulkan/vulkan_core.h>

#include "../../graphics_internal.hpp"
#include "material.hpp"
namespace graphics {
Figure::Figure(const Shape &shape, graphics::DescriptorSet &descriptor_set)
    : descriptor_set(std::move(descriptor_set)),
      uniform_buffer(this->descriptor_set, FIGURE_BINDING),
      index_count(shape.indices.size()) {
  auto &context = graphics::internal::context;
  // Vertex buffer
  const VkBufferCreateInfo vertex_buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = shape.vertices.size() * sizeof(graphics::Vertex),
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  const VmaAllocationCreateInfo vertex_buffer_allocation_info{
      .flags = (VMA_ALLOCATION_CREATE_MAPPED_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT),
      .usage = VMA_MEMORY_USAGE_AUTO};
  if (vmaCreateBuffer(context.allocator, &vertex_buffer_info,
                      &vertex_buffer_allocation_info, &vk_vertex_buffer,
                      &vk_vertex_buffer_allocation, nullptr) != VK_SUCCESS) {
    std::cerr << "Failed to create and allocate a vertex buffer\n";
  }
  if (vmaMapMemory(context.allocator, vk_vertex_buffer_allocation,
                   reinterpret_cast<void **>(&vk_vertex_buffer_memory)) !=
      VK_SUCCESS) {
    std::cerr << "Failed to map vertex buffer memory\n";
  }

  // Index buffer
  const VkBufferCreateInfo index_buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = shape.indices.size() * sizeof(uint16_t),
      .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  const VmaAllocationCreateInfo index_buffer_allocation_info{
      .flags = (VMA_ALLOCATION_CREATE_MAPPED_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT),
      .usage = VMA_MEMORY_USAGE_AUTO};
  if (vmaCreateBuffer(context.allocator, &index_buffer_info,
                      &index_buffer_allocation_info, &vk_index_buffer,
                      &vk_index_buffer_allocation, nullptr) != VK_SUCCESS) {
    std::cerr << "Failed to create and allocate a vertex buffer\n";
  }
  if (vmaMapMemory(context.allocator, vk_index_buffer_allocation,
                   reinterpret_cast<void **>(&vk_index_buffer_memory)) !=
      VK_SUCCESS) {
    std::cerr << "Failed to map index buffer memory\n";
  }
  memcpy(vk_vertex_buffer_memory, shape.vertices.data(),
         shape.vertices.size() * sizeof(graphics::Vertex));
  memcpy(vk_index_buffer_memory, shape.indices.data(),
         shape.indices.size() * sizeof(uint16_t));
  this->descriptor_set.Update();
}
Figure::~Figure() {
  auto &context = graphics::internal::context;
  if (vk_vertex_buffer != VK_NULL_HANDLE) {
    vmaUnmapMemory(context.allocator, vk_vertex_buffer_allocation);
    vmaDestroyBuffer(context.allocator, vk_vertex_buffer,
                     vk_vertex_buffer_allocation);
  }
  if (vk_index_buffer != VK_NULL_HANDLE) {

    vmaUnmapMemory(context.allocator, vk_index_buffer_allocation);
    vmaDestroyBuffer(context.allocator, vk_index_buffer,
                     vk_index_buffer_allocation);
  }
}
void Figure::Bind(VkCommandBuffer command_buffer,
                  VkPipelineLayout pipeline_layout) {

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vk_vertex_buffer, &offset);
  vkCmdBindIndexBuffer(command_buffer, vk_index_buffer, 0,
                       VK_INDEX_TYPE_UINT16);
  descriptor_set.Bind(command_buffer, pipeline_layout, FIGURE_SET_ID);
}
void Figure::Draw(VkCommandBuffer command_buffer) {
  vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}
void Figure::UpdateFigure(const std::array<float, 3> &position,
                          const std::array<float, 3> &rotation,
                          const std::array<float, 3> &scale,
                          const std::array<float, 3> &color) {

  glm::mat4 move_matrix =
      glm::translate(glm::mat4(1.0f), glm::make_vec3(position.data()));
  glm::mat4 scaling_matrix =
      glm::scale(glm::mat4(1.0f), glm::make_vec3(scale.data()));
  glm::mat4 rotation_matrix = glm::rotate(
      glm::mat4(1.0f), glm::radians(rotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
  rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation[1]),
                                glm::vec3(0.0f, 1.0f, 0.0f));
  rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation[2]),
                                glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 model = move_matrix * scaling_matrix * rotation_matrix;
  memcpy(uniform_buffer.GetPtr()->model, glm::value_ptr(model),
         sizeof(UniformData::model));
  memcpy(uniform_buffer.GetPtr()->color, color.data(), sizeof(color));
}
Figure::Figure(Figure &&other) noexcept
    : descriptor_set(std::move(other.descriptor_set)),
      uniform_buffer(std::move(other.uniform_buffer)) {
  vk_vertex_buffer = other.vk_vertex_buffer;
  vk_vertex_buffer_allocation = other.vk_vertex_buffer_allocation;
  vk_vertex_buffer_memory = other.vk_vertex_buffer_memory;
  vk_index_buffer = other.vk_index_buffer;
  vk_index_buffer_allocation = other.vk_index_buffer_allocation;
  vk_index_buffer_memory = other.vk_index_buffer_memory;
  index_count = other.index_count;

  other.vk_vertex_buffer = VK_NULL_HANDLE;
  other.vk_index_buffer = VK_NULL_HANDLE;
  other.vk_vertex_buffer_allocation = VK_NULL_HANDLE;
  other.vk_index_buffer_allocation = VK_NULL_HANDLE;
  other.vk_vertex_buffer_memory = nullptr;
  other.vk_index_buffer_memory = nullptr;
  other.index_count = 0;
}
void Figure::UpdateMaterial(const graphics::Material &material) {
  auto ptr = uniform_buffer.GetPtr();
  memcpy(&(ptr->material), &material, sizeof(ptr->material));
}
Figure &Figure::operator=(Figure &&other) noexcept {
  if (this != &other) {
    this->~Figure();
    new (this) Figure(std::move(other));
  }
  return *this;
}
}; // namespace graphics
