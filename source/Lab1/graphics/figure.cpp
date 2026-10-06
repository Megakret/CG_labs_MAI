#include "figure.hpp"

#include <iostream>
#include <string.h>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vulkan/vulkan_core.h>
namespace graphics {
Figure::Figure(const Shape &shape, VkDescriptorPool descriptor_pool,
               VkDescriptorSetLayout descriptor_layout)
    : index_count(shape.indices.size()) {
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
  // Uniform buffer
  const VkBufferCreateInfo uniform_buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = (sizeof(UniformData) + 0xf) & ~0xf,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  const VmaAllocationCreateInfo uniform_buffer_allocation_info{
      .flags = (VMA_ALLOCATION_CREATE_MAPPED_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT),
      .usage = VMA_MEMORY_USAGE_AUTO};
  if (vmaCreateBuffer(context.allocator, &uniform_buffer_info,
                      &uniform_buffer_allocation_info, &vk_uniform_buffer,
                      &vk_uniform_buffer_allocation, nullptr) != VK_SUCCESS) {
    std::cerr << "Failed to create and allocate a vertex buffer\n";
  }
  if (vmaMapMemory(context.allocator, vk_uniform_buffer_allocation,
                   reinterpret_cast<void **>(&vk_uniform_buffer_memory)) !=
      VK_SUCCESS) {
    std::cerr << "Failed to map vertex buffer memory\n";
  }
  new (vk_uniform_buffer_memory) UniformData();
  // Creating a descriptor set
  const VkDescriptorSetAllocateInfo descriptor_set_allocate_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptor_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &descriptor_layout};
  if (vkAllocateDescriptorSets(context.device, &descriptor_set_allocate_info,
                               &vk_uniform_descriptor_set) != VK_SUCCESS) {
    std::cerr << "Failed to allocate descriptor set\n";
  }
  const VkDescriptorBufferInfo descriptor_buffer_info{
      .buffer = vk_uniform_buffer, .offset = 0, .range = sizeof(UniformData)};
  std::array<VkWriteDescriptorSet, 1> descriptor_writes{VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = vk_uniform_descriptor_set,
      .dstBinding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pBufferInfo = &descriptor_buffer_info,
  }};
  vkUpdateDescriptorSets(context.device,
                         static_cast<uint32_t>(descriptor_writes.size()),
                         descriptor_writes.data(), 0, nullptr);
}
Figure::~Figure() {
  auto &context = graphics::internal::context;
  vmaUnmapMemory(context.allocator, vk_vertex_buffer_allocation);
  vmaDestroyBuffer(context.allocator, vk_vertex_buffer,
                   vk_vertex_buffer_allocation);
  vmaUnmapMemory(context.allocator, vk_index_buffer_allocation);
  vmaDestroyBuffer(context.allocator, vk_index_buffer,
                   vk_index_buffer_allocation);
  vmaUnmapMemory(context.allocator, vk_uniform_buffer_allocation);
  vmaDestroyBuffer(context.allocator, vk_uniform_buffer,
                   vk_uniform_buffer_allocation);
}
void Figure::Bind(VkCommandBuffer command_buffer) {

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vk_vertex_buffer, &offset);
  vkCmdBindIndexBuffer(command_buffer, vk_index_buffer, 0,
                       VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          graphics::triangle_pipeline_layout, 0, 1,
                          &vk_uniform_descriptor_set, 0, nullptr);
}
void Figure::Draw(VkCommandBuffer command_buffer) {
  vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}
void Figure::UpdateFigure(const glm::mat4& projection_matrix,
                  const std::array<float, 3> &position,
                  const std::array<float, 3> &rotation,
                  const std::array<float, 3> &scale,
                  const std::array<float, 3> &color) {

  glm::mat4 move_matrix = glm::translate(
      glm::mat4(1.0f), glm::make_vec3(position.data()));
  glm::mat4 scaling_matrix =
      glm::scale(glm::mat4(1.0f), glm::make_vec3(scale.data()));
  glm::mat4 rotation_matrix =
      glm::rotate(glm::mat4(1.0f), glm::radians(rotation[0]),
                  glm::vec3(1.0f, 0.0f, 0.0f));
  rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation[1]),
                         glm::vec3(0.0f, 1.0f, 0.0f));
  rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation[2]),
                         glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 move_and_projection = projection_matrix * move_matrix* scaling_matrix;
  glm::mat4 local_coords_matrix = rotation_matrix;
  memcpy(vk_uniform_buffer_memory->transform_projection_matrix,
         glm::value_ptr(move_and_projection),
         sizeof(UniformData::transform_projection_matrix));
  memcpy(vk_uniform_buffer_memory->local_coords_matrix,
         glm::value_ptr(local_coords_matrix),
         sizeof(UniformData::local_coords_matrix));
  memcpy(vk_uniform_buffer_memory->color, color.data(), sizeof(color));
}
}; // namespace graphics
