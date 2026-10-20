#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace graphics {
class DescriptorSet {
public:
  DescriptorSet(VkDescriptorSet set, VkDescriptorPool pool);
  void DeferUpdate(size_t binding, VkDescriptorType descriptor_type,
                   VkDescriptorBufferInfo buffer_info);
  void Update();
  void Bind(VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout,
            uint32_t set);

private:
  VkDescriptorSet set;
  VkDescriptorPool pool;
  std::vector<VkWriteDescriptorSet> writes;
  std::vector<std::unique_ptr<VkDescriptorBufferInfo>> buffer_infos;
};
class DescriptorPool {
public:
  DescriptorPool(const std::vector<VkDescriptorPoolSize> &pool_sizes,
                 size_t max_sets);
  DescriptorSet AllocateSet(VkDescriptorSetLayout set_layout);

private:
  VkDescriptorPool pool;
};
}; // namespace graphics
