#pragma once
#include <span>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace graphics {
extern VkPipelineLayout triangle_pipeline_layout;
extern VkPipeline triangle_pipeline;
extern VkDescriptorSetLayout g_scene_descriptor_layout;
extern VkDescriptorSetLayout g_figure_descriptor_layout;
extern const size_t STORAGE_BUFFER_COUNT;
extern const size_t SCENE_DESCRIPTOR_COUNT;

bool load_shared_module(const char *filepath, VkShaderModule *vk_shared_module);
void init_pipelines();
struct Vertex {
  std::array<float, 3> position;
  std::array<float, 3> color;
  std::array<float, 3> normal;
};

class PipelineBuilder {
public:
  std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
  VkPipelineVertexInputStateCreateInfo vertex_input_info;
  VkPipelineInputAssemblyStateCreateInfo input_assembly;
  VkViewport viewport;
  VkRect2D scissor;
  VkPipelineRasterizationStateCreateInfo rasterizer;
  VkPipelineColorBlendAttachmentState color_blend_attachment;
  VkPipelineMultisampleStateCreateInfo multisampling;
  VkPipelineLayout pipeline_layout;

  VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};
VkPipelineShaderStageCreateInfo
pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                                  VkShaderModule shaderModule);
VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(
    std::span<const VkVertexInputBindingDescription> bindings,
    std::span<const VkVertexInputAttributeDescription> attributes);
VkPipelineInputAssemblyStateCreateInfo
input_assembly_create_info(VkPrimitiveTopology topology);
VkPipelineRasterizationStateCreateInfo
rasterization_state_create_info(VkPolygonMode polygonMode);
VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
VkPipelineColorBlendAttachmentState color_blend_attachment_state();
VkPipelineLayoutCreateInfo
pipeline_layout_create_info(std::span<VkDescriptorSetLayout> layouts);
VkDescriptorSetLayout create_descriptor_set_layout(
    std::span<const VkDescriptorSetLayoutBinding> bindings);
} // namespace graphics
