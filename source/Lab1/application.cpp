#include "application.hpp"

#include <cstring>
#include <imgui.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

#include "../graphics_internal.hpp"
#include "graphics.hpp"
namespace application {
VkBuffer vk_vertex_buffer;
VmaAllocation vk_vertex_buffer_allocation;
graphics::Vertex *vk_vertex_buffer_memory;
std::vector<graphics::Vertex> vertices{
    {{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
};
float phi = 0;
bool initialize() {
  auto &context = graphics::internal::context;
  graphics::init_pipelines();
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
}
std::vector<bool> states{true, true, true};
double prev_time = 0;
void update([[maybe_unused]] double time) {
  if (prev_time == 0) {
    prev_time = time;
  }
  double dt = time - prev_time;
  prev_time = time;
  for (auto &vertice : vertices) {
    for (int i = 0; i < 3; i++) {
      if (states[i]) {
        vertice.color[i] += 1.0f * dt;
      } else {
        vertice.color[i] -= 1.0f * dt;
      }
      if (vertice.color[i] > 1.0f) {
        states[i] = false;
      }
      if (vertice.color[i] < 0.0f) {
        states[i] = true;
      }
    }
  }
}

void render(const graphics::internal::FrameData &fd) {
  auto &context = graphics::internal::context;
  vkResetCommandBuffer(fd.command_buffer, 1);
  VkCommandBufferBeginInfo cmd_begin_info = {};
  cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_begin_info.pNext = nullptr;
  VkClearValue clearValue;
  clearValue.color = {{0.0f, 0.0f, 120.0f, 1.0f}};
  VkRenderPassBeginInfo rp_info{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = context.render_pass,
      .framebuffer = fd.framebuffer,
      .renderArea = {.extent = context.swapchain_extent},
      .clearValueCount = 1,
      .pClearValues = &clearValue};
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
  vkCmdDraw(fd.command_buffer, vertices.size(), 1, 0, 0);

  // finalize the render pass
  vkCmdEndRenderPass(fd.command_buffer);
  vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
