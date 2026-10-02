#include "application.hpp"

#include <cstring>
#include <imgui.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

#include "../graphics_internal.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "graphics.hpp"
namespace application {
VkBuffer vk_vertex_buffer;
VmaAllocation vk_vertex_buffer_allocation;
graphics::Vertex *vk_vertex_buffer_memory;
// Unit cube (half-extent 1, spanning -1..1), 36 vertices:
// 6 faces * 2 triangles * 3 vertices, one color per face.
std::vector<graphics::Vertex> vertices{
    // +Z (front) - magenta
    {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
    // -Z (back) - cyan
    {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    // +X (right) - red
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    // -X (left) - green
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
    // +Y (top) - blue
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
    // -Y (bottom) - yellow
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
};
struct GlobalUniforms {
  float matrix[4][4];
};
VkBuffer vk_uniform_buffer;
VmaAllocation vk_uniform_buffer_allocation;
GlobalUniforms *vk_uniform_buffer_memory;
VkDescriptorPool vk_descriptor_pool;
VkDescriptorSet vk_uniform_descriptor_set;
bool initialize() {
  auto &context = graphics::internal::context;
  graphics::init_pipelines();

  // Vertex buffer
  const VkBufferCreateInfo buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = vertices.size() * sizeof(graphics::Vertex),
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  const VmaAllocationCreateInfo vertex_buffer_allocation_info{
      .flags = (VMA_ALLOCATION_CREATE_MAPPED_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT),
      .usage = VMA_MEMORY_USAGE_AUTO};
  if (vmaCreateBuffer(context.allocator, &buffer_info,
                      &vertex_buffer_allocation_info, &vk_vertex_buffer,
                      &vk_vertex_buffer_allocation, nullptr) != VK_SUCCESS) {
    std::cerr << "Failed to create and allocate a vertex buffer\n";
    return false;
  }
  if (vmaMapMemory(context.allocator, vk_vertex_buffer_allocation,
                   reinterpret_cast<void **>(&vk_vertex_buffer_memory)) !=
      VK_SUCCESS) {
    std::cerr << "Failed to map vertex buffer memory\n";
    return false;
  }
  // Uniform buffer
  const VkBufferCreateInfo uniform_buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = (sizeof(GlobalUniforms) + 0xf) & ~0xf,
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
    return false;
  }
  if (vmaMapMemory(context.allocator, vk_uniform_buffer_allocation,
                   reinterpret_cast<void **>(&vk_uniform_buffer_memory)) !=
      VK_SUCCESS) {
    std::cerr << "Failed to map vertex buffer memory\n";
    return false;
  }

  const VkDescriptorPoolSize descriptor_pool_sizes[]{
      VkDescriptorPoolSize{
         .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 4
      }
  };
  const VkDescriptorPoolCreateInfo descriptor_pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = sizeof(descriptor_pool_info) / sizeof(descriptor_pool_sizes[0]),
      .pPoolSizes = descriptor_pool_sizes
  };
  if (vkCreateDescriptorPool(context.device, &descriptor_pool_info, nullptr, &vk_descriptor_pool) != VK_SUCCESS){
      std::cerr << "Failed to create a descriptor pool\n";
      return false;
  }
  const VkDescriptorSetAllocateInfo descriptor_set_allocate_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = vk_descriptor_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &graphics::descriptor_set_layout,
  };
  if(vkAllocateDescriptorSets(context.device, &descriptor_set_allocate_info, &vk_uniform_descriptor_set) != VK_SUCCESS){
      std::cerr << "Failed to allocate descriptor set\n";
      return false;
  }
  const VkDescriptorBufferInfo descriptor_buffer_info{
      .buffer = vk_uniform_buffer,
      .offset = 0,
      .range = sizeof(GlobalUniforms)
  };
  const VkWriteDescriptorSet descriptor_writes[]{
      VkWriteDescriptorSet{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = vk_uniform_descriptor_set,
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &descriptor_buffer_info,
      }
  };
  vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(sizeof(descriptor_writes) / sizeof(descriptor_writes[0])), descriptor_writes, 0, nullptr);

  return true;
}

void shutdown() {
  auto &context = graphics::internal::context;
  vkQueueWaitIdle(context.graphics_queue);
  vkDestroyPipeline(context.device, graphics::triangle_pipeline, nullptr);
  vkDestroyPipelineLayout(context.device, graphics::triangle_pipeline_layout,
                          nullptr);
  vmaUnmapMemory(context.allocator, vk_vertex_buffer_allocation);
  vmaDestroyBuffer(context.allocator, vk_vertex_buffer,
                   vk_vertex_buffer_allocation);
  vmaUnmapMemory(context.allocator, vk_uniform_buffer_allocation);
  vmaDestroyBuffer(context.allocator, vk_uniform_buffer, vk_uniform_buffer_allocation);
}
std::vector<bool> states{true, true, true};
double prev_time = 0;
const float FOV = 70.0f;
void update([[maybe_unused]] double time) {
  auto &context = graphics::internal::context;
  glm::mat4 perspective = glm::perspectiveFov(
      FOV, static_cast<float>(context.swapchain_extent.width),
      static_cast<float>(context.swapchain_extent.height), 0.1f, 100.0f);
  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(glm::vec3(0.0f, 0.0f, -20.0f)));
  glm::mat4 total_matrix = perspective * model;
  memcpy(vk_uniform_buffer_memory->matrix, glm::value_ptr(total_matrix),
         sizeof(GlobalUniforms::matrix));
}

void render(const graphics::internal::FrameData &fd) {
  auto &context = graphics::internal::context;
  vkResetCommandBuffer(fd.command_buffer, 1);
  VkCommandBufferBeginInfo cmd_begin_info = {};
  cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_begin_info.pNext = nullptr;
  VkClearValue clearValues []{
      {.color = {{0.0f, 0.0f, 120.0f, 1.0f}}},
      {.depthStencil = {1.0f, 0}},
  };
  VkRenderPassBeginInfo rp_info{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = context.render_pass,
      .framebuffer = fd.framebuffer,
      .renderArea = {.extent = context.swapchain_extent},
      .clearValueCount = 2,
      .pClearValues = clearValues};
  vkBeginCommandBuffer(fd.command_buffer, &cmd_begin_info);
  vkCmdBeginRenderPass(fd.command_buffer, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
  const VkViewport viewport{
      .x = 0,
      .y = 0,
      .width = static_cast<float>(context.swapchain_extent.width),
      .height = static_cast<float>(context.swapchain_extent.height),
      .minDepth = 0,
      .maxDepth = 1,
  };
  const VkRect2D scissor{.extent = context.swapchain_extent};

  vkCmdSetViewport(fd.command_buffer, 0, 1, &viewport);
  vkCmdSetScissor(fd.command_buffer, 0, 1, &scissor);
  vkCmdBindPipeline(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphics::triangle_pipeline);
  memcpy(vk_vertex_buffer_memory, vertices.data(),
         vertices.size() * sizeof(graphics::Vertex));
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, &vk_vertex_buffer, &offset);
  vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics::triangle_pipeline_layout, 0, 1, &vk_uniform_descriptor_set, 0, nullptr);
  vkCmdDraw(fd.command_buffer, vertices.size(), 1, 0, 0);

  // finalize the render pass
  vkCmdEndRenderPass(fd.command_buffer);
  vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
