#pragma once

#include <array>
#include <iostream>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "../../graphics_internal.hpp"

namespace graphics {
template <typename BufferT> class UniformBuffer {
private:
  VkBuffer vk_uniform_buffer;
  VmaAllocation vk_uniform_buffer_allocation;
  BufferT *vk_uniform_buffer_memory;
  VkDescriptorSet vk_uniform_descriptor_set;

public:
  UniformBuffer(VkDescriptorPool pool, VkDescriptorSetLayout layout, uint32_t binding);
  UniformBuffer(const UniformBuffer<BufferT> &) = delete;
  UniformBuffer<BufferT>& operator=(const UniformBuffer<BufferT> &) = delete;
  UniformBuffer(UniformBuffer<BufferT> &&) noexcept;
  UniformBuffer<BufferT>& operator=(UniformBuffer<BufferT> &&) noexcept;
  ~UniformBuffer();

  BufferT *GetPtr();
  void Bind(VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout, uint32_t set);
};
template <typename BufferT>
UniformBuffer<BufferT>::UniformBuffer(VkDescriptorPool pool,
                                      VkDescriptorSetLayout layout, uint32_t binding) {
  auto &context = graphics::internal::context;
  const VkBufferCreateInfo uniform_buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = (sizeof(BufferT) + 0xf) & ~0xf,
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
  new (vk_uniform_buffer_memory) BufferT();
  // Creating a descriptor set
  const VkDescriptorSetAllocateInfo descriptor_set_allocate_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout};
  if (vkAllocateDescriptorSets(context.device, &descriptor_set_allocate_info,
                               &vk_uniform_descriptor_set) != VK_SUCCESS) {
    std::cerr << "Failed to allocate descriptor set\n";
  }
  const VkDescriptorBufferInfo descriptor_buffer_info{
      .buffer = vk_uniform_buffer, .offset = 0, .range = sizeof(BufferT)};
  std::array<VkWriteDescriptorSet, 1> descriptor_writes{VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = vk_uniform_descriptor_set,
      .dstBinding = binding,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pBufferInfo = &descriptor_buffer_info,
  }};
  vkUpdateDescriptorSets(context.device,
                         static_cast<uint32_t>(descriptor_writes.size()),
                         descriptor_writes.data(), 0, nullptr);
}

template <typename T> T *UniformBuffer<T>::GetPtr() {
  return vk_uniform_buffer_memory;
}
template <typename BufferT>
UniformBuffer<BufferT>::UniformBuffer(UniformBuffer<BufferT> &&other) noexcept {

  vk_uniform_buffer = other.vk_uniform_buffer;
  vk_uniform_buffer_allocation = other.vk_uniform_buffer_allocation;
  vk_uniform_buffer_memory = other.vk_uniform_buffer_memory;
  vk_uniform_descriptor_set = other.vk_uniform_descriptor_set;
  other.vk_uniform_buffer = VK_NULL_HANDLE;
  other.vk_uniform_buffer_allocation = VK_NULL_HANDLE;
  other.vk_uniform_buffer_memory = nullptr;
  other.vk_uniform_descriptor_set = VK_NULL_HANDLE;
}
template<typename BufferT>
UniformBuffer<BufferT>::~UniformBuffer() {
    auto &context = graphics::internal::context;
    vmaUnmapMemory(context.allocator, vk_uniform_buffer_allocation);
    vmaDestroyBuffer(context.allocator, vk_uniform_buffer,
                     vk_uniform_buffer_allocation);
}
template<typename BufferT>
UniformBuffer<BufferT>& UniformBuffer<BufferT>::operator=(UniformBuffer<BufferT> && other) noexcept{
    if (this != &other) {
      this->~UniformBuffer<BufferT>();
      UniformBuffer<BufferT>(std::move(other));
    }
    return *this;
}
template<typename BufferT>
void UniformBuffer<BufferT>::Bind(VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout, uint32_t set){
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout, set, 1,
                            &vk_uniform_descriptor_set, 0, nullptr);
}
} // namespace graphics
