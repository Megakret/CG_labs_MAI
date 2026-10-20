#include "descriptors.hpp"

#include <iostream>

#include <vulkan/vulkan_core.h>

#include "../../graphics_internal.hpp"

namespace graphics {
DescriptorSet::DescriptorSet(VkDescriptorSet set, VkDescriptorPool pool)
    : set(set), pool(pool) {}
void DescriptorSet::DeferUpdate(size_t binding,
                                VkDescriptorType descriptor_type,
                                VkDescriptorBufferInfo buffer_info) {
  buffer_infos.push_back(std::make_unique<VkDescriptorBufferInfo>(buffer_info));
  writes.push_back(VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set,
      .dstBinding = static_cast<uint32_t>(binding),
      .descriptorCount = 1,
      .descriptorType = descriptor_type,
      .pBufferInfo = buffer_infos[buffer_infos.size() - 1].get(),
  });
}
void DescriptorSet::Update() {
  auto &context = graphics::internal::context;
  vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);
  writes.clear();
  buffer_infos.clear();
}
void DescriptorSet::Bind(VkCommandBuffer command_buffer,
                         VkPipelineLayout pipeline_layout, uint32_t set_id) {
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout, set_id, 1, &set, 0, nullptr);
}
DescriptorPool::DescriptorPool(
    const std::vector<VkDescriptorPoolSize> &pool_sizes, size_t max_sets) {
  auto &context = graphics::internal::context;
  const VkDescriptorPoolCreateInfo descriptor_pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = static_cast<uint32_t>(max_sets),
      .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
      .pPoolSizes = pool_sizes.data()};
  if (vkCreateDescriptorPool(context.device, &descriptor_pool_info, nullptr,
                             &pool) != VK_SUCCESS) {
    std::cerr << "Failed to create a descriptor pool\n";
  }
}
DescriptorSet DescriptorPool::AllocateSet(VkDescriptorSetLayout set_layout) {
  auto &context = graphics::internal::context;
  const VkDescriptorSetAllocateInfo descriptor_set_allocate_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &set_layout};
  VkDescriptorSet vk_set;
  if (vkAllocateDescriptorSets(context.device, &descriptor_set_allocate_info,
                               &vk_set) != VK_SUCCESS) {
    std::cerr << "Failed to allocate descriptor set\n";
  }
  return DescriptorSet(vk_set, pool);
}
}; // namespace graphics
