#include "graphics.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "../../graphics_internal.hpp"

namespace graphics {

const char *VERTEX_SHADER_PATH = "shaders/triangle.vert.spv";
const char *FRAGMENT_SHADER_PATH = "shaders/fragment.frag.spv";

VkPipelineLayout triangle_pipeline_layout;
VkPipeline triangle_pipeline;
VkDescriptorSetLayout descriptor_set_layout;

bool load_shader_module(const char *filepath,
                        VkShaderModule *out_shader_module) {
  std::ifstream file(filepath, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "file couldn't open\n";
    return false;
  }
  size_t filesize = static_cast<size_t>(file.tellg());
  std::vector<uint32_t> buffer(filesize / sizeof(uint32_t));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), filesize);
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = nullptr;
  create_info.codeSize = filesize;
  create_info.pCode = buffer.data();

  VkShaderModule shader_module;
  if (vkCreateShaderModule(graphics::internal::context.device, &create_info,
                           nullptr, &shader_module) != VK_SUCCESS) {
    return false;
  }
  *out_shader_module = shader_module;
  return true;
}
void init_pipelines() {
  VkShaderModule triangleFragShader;
  if (!load_shader_module(FRAGMENT_SHADER_PATH, &triangleFragShader)) {
    std::cerr << "Error when building the triangle fragment shader module"
              << std::endl;
  } else {
    std::cout << "Triangle fragment shader successfully loaded" << std::endl;
  }

  VkShaderModule triangle_vertex_shader;
  if (!load_shader_module(VERTEX_SHADER_PATH, &triangle_vertex_shader)) {
    std::cerr << "Error when building the triangle vertex shader module"
              << std::endl;

  } else {
    std::cout << "Triangle vertex shader successfully loaded" << std::endl;
  }
  const VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[]{
      VkDescriptorSetLayoutBinding{
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      }};
  descriptor_set_layout = create_descriptor_set_layout(std::span(descriptor_set_layout_bindings, 1));
  VkPipelineLayoutCreateInfo pipeline_layout_info =
      pipeline_layout_create_info(&descriptor_set_layout);
  if (vkCreatePipelineLayout(graphics::internal::context.device,
                             &pipeline_layout_info, nullptr,
                             &triangle_pipeline_layout) != VK_SUCCESS) {
    std::cerr << "Couldn't create pipeline layout create info";
  }
  // layout and shader modules creation

  // build the stage-create-info for both vertex and fragment stages. This lets
  // the pipeline know the shader modules per stage
  PipelineBuilder pipelineBuilder;

  pipelineBuilder.shader_stages.push_back(pipeline_shader_stage_create_info(
      VK_SHADER_STAGE_VERTEX_BIT, triangle_vertex_shader));

  pipelineBuilder.shader_stages.push_back(pipeline_shader_stage_create_info(
      VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

  const std::array<VkVertexInputBindingDescription, 1> bindings{
      VkVertexInputBindingDescription{.binding = 0,
                                      .stride = sizeof(Vertex),
                                      .inputRate =
                                          VK_VERTEX_INPUT_RATE_VERTEX}};
  const std::array<VkVertexInputAttributeDescription, 2> attributes{
      VkVertexInputAttributeDescription{.location = 0,
                                        .binding = 0,
                                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                                        .offset = offsetof(Vertex, position)},
      VkVertexInputAttributeDescription{.location = 1,
                                        .binding = 0,
                                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                                        .offset = offsetof(Vertex, color)}};
  pipelineBuilder.vertex_input_info = vertex_input_state_create_info(
      std::span(bindings), std::span(attributes));

  // input assembly is the configuration for drawing triangle lists, strips, or
  // individual points. we are just going to draw triangle list
  pipelineBuilder.input_assembly =
      input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  // build viewport and scissor from the swapchain extents
  pipelineBuilder.viewport.x = 0.0f;
  pipelineBuilder.viewport.y = 0.0f;
  pipelineBuilder.viewport.width =
      graphics::internal::context.swapchain_extent.width;
  pipelineBuilder.viewport.height =
      graphics::internal::context.swapchain_extent.height;
  pipelineBuilder.viewport.minDepth = 0.0f;
  pipelineBuilder.viewport.maxDepth = 1.0f;

  pipelineBuilder.scissor.offset = {0, 0};
  pipelineBuilder.scissor.extent = graphics::internal::context.swapchain_extent;

  // configure the rasterizer to draw filled triangles
  pipelineBuilder.rasterizer =
      rasterization_state_create_info(VK_POLYGON_MODE_FILL);

  // we don't use multisampling, so just run the default one
  pipelineBuilder.multisampling = multisampling_state_create_info();

  // a single blend attachment with no blending and writing to RGBA
  pipelineBuilder.color_blend_attachment = color_blend_attachment_state();

  // use the triangle layout we created
  pipelineBuilder.pipeline_layout = triangle_pipeline_layout;

  // finally build the pipeline
  triangle_pipeline =
      pipelineBuilder.build_pipeline(graphics::internal::context.device,
                                     graphics::internal::context.render_pass);
}

VkPipelineShaderStageCreateInfo
pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                                  VkShaderModule shaderModule) {

  VkPipelineShaderStageCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.pNext = nullptr;

  // shader stage
  info.stage = stage;
  // module containing the code for this shader stage
  info.module = shaderModule;
  // the entry point of the shader
  info.pName = "main";
  return info;
}
VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(
    std::span<const VkVertexInputBindingDescription> bindings,
    std::span<const VkVertexInputAttributeDescription> attributes) {
  VkPipelineVertexInputStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.pVertexBindingDescriptions = bindings.data();
  info.vertexBindingDescriptionCount = bindings.size();
  info.pVertexAttributeDescriptions = attributes.data();
  info.vertexAttributeDescriptionCount = attributes.size();
  return info;
}

VkPipelineInputAssemblyStateCreateInfo
input_assembly_create_info(VkPrimitiveTopology topology) {
  VkPipelineInputAssemblyStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  info.pNext = nullptr;

  info.topology = topology;
  // we are not going to use primitive restart on the entire tutorial so leave
  // it on false
  info.primitiveRestartEnable = VK_FALSE;
  return info;
}
VkPipelineRasterizationStateCreateInfo
rasterization_state_create_info(VkPolygonMode polygonMode) {
  VkPipelineRasterizationStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  info.pNext = nullptr;

  info.depthClampEnable = VK_FALSE;
  // discards all primitives before the rasterization stage if enabled which we
  // don't want
  info.rasterizerDiscardEnable = VK_FALSE;

  info.polygonMode = polygonMode;
  info.lineWidth = 1.0f;
  // no backface cull
  info.cullMode = VK_CULL_MODE_NONE;
  info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  // no depth bias
  info.depthBiasEnable = VK_FALSE;
  info.depthBiasConstantFactor = 0.0f;
  info.depthBiasClamp = 0.0f;
  info.depthBiasSlopeFactor = 0.0f;

  return info;
}
VkPipelineMultisampleStateCreateInfo multisampling_state_create_info() {
  VkPipelineMultisampleStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  info.pNext = nullptr;

  info.sampleShadingEnable = VK_FALSE;
  // multisampling defaulted to no multisampling (1 sample per pixel)
  info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  info.minSampleShading = 1.0f;
  info.pSampleMask = nullptr;
  info.alphaToCoverageEnable = VK_FALSE;
  info.alphaToOneEnable = VK_FALSE;
  return info;
}
VkPipelineColorBlendAttachmentState color_blend_attachment_state() {
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  return colorBlendAttachment;
}
VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass) {
  // make viewport state from our stored viewport and scissor.
  // at the moment we won't support multiple viewports or scissors
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = nullptr;

  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  // in graphics::PipelineBuilder::build_pipeline
  static const VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  dynamic_state.pDynamicStates = dynamic_states;

  // setup dummy color blending. We aren't using transparent objects yet
  // the blending is just "no blend", but we do write to the color attachment
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.pNext = nullptr;

  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &color_blend_attachment;
  const VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
     .depthTestEnable = VK_TRUE,
     .depthWriteEnable = VK_TRUE,
     .depthCompareOp = VK_COMPARE_OP_LESS
  };
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = nullptr;

  pipelineInfo.stageCount = shader_stages.size();
  pipelineInfo.pStages = shader_stages.data();
  pipelineInfo.pVertexInputState = &vertex_input_info;
  pipelineInfo.pDepthStencilState = &depth_stencil_info;
  pipelineInfo.pInputAssemblyState = &input_assembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = pipeline_layout;
  pipelineInfo.renderPass = pass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.pDynamicState = &dynamic_state;

  // it's easy to error out on create graphics pipeline, so we handle it a bit
  // better than the common VK_CHECK case
  VkPipeline newPipeline;
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &newPipeline) != VK_SUCCESS) {
    std::cout << "failed to create pipeline\n";
    return VK_NULL_HANDLE; // failed to create graphics pipeline
  } else {
    return newPipeline;
  }
}
VkPipelineLayoutCreateInfo pipeline_layout_create_info(VkDescriptorSetLayout* descriptor_set_layout) {
  VkPipelineLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.pNext = nullptr;

  info.flags = 0;
  info.setLayoutCount = 1;
  info.pSetLayouts = descriptor_set_layout;
  info.pushConstantRangeCount = 0;
  info.pPushConstantRanges = nullptr;
  return info;
}
VkDescriptorSetLayout create_descriptor_set_layout(std::span<const VkDescriptorSetLayoutBinding> bindings){
    const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    VkDescriptorSetLayout descriptor_set_layout;
    if (vkCreateDescriptorSetLayout(internal::context.device,
                                    &descriptor_set_layout_info, nullptr,
                                    &descriptor_set_layout) != VK_SUCCESS) {
      std::cerr << "Coudln't create descriptor set layout\n";
    }
    return descriptor_set_layout;
}
} // namespace graphics
