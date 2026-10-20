#pragma once

#include <array>
#include <iostream>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "../../graphics_internal.hpp"
#include "descriptors.hpp"

namespace graphics {
template <typename BufferT> class UniformBuffer {
private:
  VkBuffer vk_uniform_buffer;
  VmaAllocation vk_uniform_buffer_allocation;
  BufferT *vk_uniform_buffer_memory;

public:
  UniformBuffer(DescriptorSet &descriptor_set, uint32_t binding);
  UniformBuffer(const UniformBuffer<BufferT> &) = delete;
  UniformBuffer<BufferT> &operator=(const UniformBuffer<BufferT> &) = delete;
  UniformBuffer(UniformBuffer<BufferT> &&) noexcept;
  UniformBuffer<BufferT> &operator=(UniformBuffer<BufferT> &&) noexcept;
  ~UniformBuffer();

  BufferT *GetPtr();
};
template <typename BufferT>
UniformBuffer<BufferT>::UniformBuffer(DescriptorSet &descriptor_set,
                                      uint32_t binding) {
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
  const VkDescriptorBufferInfo descriptor_buffer_info{
      .buffer = vk_uniform_buffer, .offset = 0, .range = sizeof(BufferT)};
  descriptor_set.DeferUpdate(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                             descriptor_buffer_info);
}

template <typename T> T *UniformBuffer<T>::GetPtr() {
  return vk_uniform_buffer_memory;
}
template <typename BufferT>
UniformBuffer<BufferT>::UniformBuffer(UniformBuffer<BufferT> &&other) noexcept {

  vk_uniform_buffer = other.vk_uniform_buffer;
  vk_uniform_buffer_allocation = other.vk_uniform_buffer_allocation;
  vk_uniform_buffer_memory = other.vk_uniform_buffer_memory;
  other.vk_uniform_buffer = VK_NULL_HANDLE;
  other.vk_uniform_buffer_allocation = VK_NULL_HANDLE;
  other.vk_uniform_buffer_memory = nullptr;
}
template <typename BufferT> UniformBuffer<BufferT>::~UniformBuffer() {
  auto &context = graphics::internal::context;
  if (vk_uniform_buffer != VK_NULL_HANDLE) {
    vmaUnmapMemory(context.allocator, vk_uniform_buffer_allocation);
    vmaDestroyBuffer(context.allocator, vk_uniform_buffer,
                     vk_uniform_buffer_allocation);
  }
}
template <typename BufferT>
UniformBuffer<BufferT> &
UniformBuffer<BufferT>::operator=(UniformBuffer<BufferT> &&other) noexcept {
  if (this != &other) {
    this->~UniformBuffer<BufferT>();
    new (this) UniformBuffer<BufferT>(std::move(other));
  }
  return *this;
}
} // namespace graphics
