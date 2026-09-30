#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace graphics {
extern VkPipelineLayout triangle_pipeline_layout;
extern VkPipeline triangle_pipeline;

bool load_shared_module(const char *filepath, VkShaderModule *vk_shared_module);
void init_pipelines();

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
VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
VkPipelineInputAssemblyStateCreateInfo
input_assembly_create_info(VkPrimitiveTopology topology);
VkPipelineRasterizationStateCreateInfo
rasterization_state_create_info(VkPolygonMode polygonMode);
VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
VkPipelineColorBlendAttachmentState color_blend_attachment_state();
VkPipelineLayoutCreateInfo pipeline_layout_create_info();
} // namespace graphics
