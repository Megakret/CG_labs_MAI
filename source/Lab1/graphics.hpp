#include <span>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace graphics {
extern VkPipelineLayout triangle_pipeline_layout;
extern VkPipeline triangle_pipeline;
extern VkDescriptorSetLayout descriptor_set_layout;

bool load_shared_module(const char *filepath, VkShaderModule *vk_shared_module);
void init_pipelines();
struct Vertex {
  float position[3];
  float color[3];
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
VkPipelineLayoutCreateInfo pipeline_layout_create_info(VkDescriptorSetLayout* layout);
VkDescriptorSetLayout create_descriptor_set_layout(std::span<const VkDescriptorSetLayoutBinding> bindings);
} // namespace graphics
