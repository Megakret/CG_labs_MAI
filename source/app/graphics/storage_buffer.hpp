#pragma once

#include <array>
#include <iostream>
#include <span>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "../../graphics_internal.hpp"
#include "descriptors.hpp"

namespace graphics {
template <typename T> class StorageBuffer {
private:
  VkBuffer vk_buffer;
  VmaAllocation vk_buffer_allocation;
  std::span<T> buffer_memory;
  size_t length;

public:
  StorageBuffer(DescriptorSet &set, size_t max_length, size_t binding);
  StorageBuffer(const StorageBuffer<T> &) = delete;
  StorageBuffer<T> &operator=(const StorageBuffer<T> &) = delete;
  StorageBuffer(StorageBuffer<T> &&) noexcept;
  StorageBuffer<T> &operator=(StorageBuffer<T> &&) noexcept;
  ~StorageBuffer();

  std::span<T> GetSpan();
  VkDescriptorSet GetDescriptorSet();
};
template <typename T>
StorageBuffer<T>::StorageBuffer(DescriptorSet &set, size_t max_length,
                                size_t binding)
    : length(max_length) {
  auto &context = graphics::internal::context;
  uint32_t raw_size = sizeof(T) * length;
  const VkBufferCreateInfo buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = (raw_size + 0xf) & ~0xf,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  const VmaAllocationCreateInfo buffer_allocation_info{
      .flags = (VMA_ALLOCATION_CREATE_MAPPED_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT),
      .usage = VMA_MEMORY_USAGE_AUTO};
  if (vmaCreateBuffer(context.allocator, &buffer_info, &buffer_allocation_info,
                      &vk_buffer, &vk_buffer_allocation,
                      nullptr) != VK_SUCCESS) {
    std::cerr << "Failed to create and allocate a storage buffer\n";
  }
  T *storage_buffer_mem;
  if (vmaMapMemory(context.allocator, vk_buffer_allocation,
                   reinterpret_cast<void **>(&storage_buffer_mem)) !=
      VK_SUCCESS) {
    std::cerr << "Failed to map storage buffer memory\n";
  }
  for (size_t i = 0; i < length; i++) {
    new (&storage_buffer_mem[i]) T();
  }
  buffer_memory = std::span(storage_buffer_mem, length);

  const VkDescriptorBufferInfo descriptor_buffer_info{
      .buffer = vk_buffer, .offset = 0, .range = length * sizeof(T)};
  set.DeferUpdate(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                  descriptor_buffer_info);
}

template <typename T> std::span<T> StorageBuffer<T>::GetSpan() {
  return buffer_memory;
}
template <typename T>
StorageBuffer<T>::StorageBuffer(StorageBuffer<T> &&other) noexcept {

  vk_buffer = other.vk_buffer;
  vk_buffer_allocation = other.vk_buffer_allocation;
  buffer_memory = std::move(other.buffer_memory);
  other.vk_buffer = VK_NULL_HANDLE;
  other.vk_buffer_allocation = VK_NULL_HANDLE;
}
template <typename T> StorageBuffer<T>::~StorageBuffer() {
  auto &context = graphics::internal::context;
  if (vk_buffer != VK_NULL_HANDLE) {
    vmaUnmapMemory(context.allocator, vk_buffer_allocation);
    vmaDestroyBuffer(context.allocator, vk_buffer, vk_buffer_allocation);
  }
}
template <typename T>
StorageBuffer<T> &
StorageBuffer<T>::operator=(StorageBuffer<T> &&other) noexcept {
  if (this != &other) {
    this->~StorageBuffer<T>();
    new (this) StorageBuffer<T>(std::move(other));
  }
  return *this;
}
} // namespace graphics
