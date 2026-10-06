#include "application.hpp"

#include <cstring>
#include <imgui.h>
#include <iostream>

#include <vulkan/vulkan_core.h>
#include "../graphics_internal.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "graphics.hpp"
#include "animation.hpp"
namespace application {
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
VkBuffer vk_vertex_buffer;
VmaAllocation vk_vertex_buffer_allocation;
graphics::Vertex *vk_vertex_buffer_memory;

VkBuffer vk_index_buffer;
VmaAllocation vk_index_buffer_allocation;
uint16_t *vk_index_buffer_memory;
// Regular octahedron: 6 unique vertices at +/-1 on each axis.
std::vector<graphics::Vertex> vertices{
    {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},   // 0: +X
    {{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},  // 1: -X
    {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},   // 2: +Y
    {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},  // 3: -Y
    {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},   // 4: +Z
    {{0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},  // 5: -Z
};
// 8 triangular faces, each wound CCW when viewed from outside.
const std::vector<uint16_t> indexes{
    0, 2, 4, // +X +Y +Z
    0, 4, 3, // +X -Y +Z
    1, 3, 4, // -X -Y +Z
    1, 4, 2, // -X +Y +Z
    1, 5, 3, // -X -Y -Z
    0, 3, 5, // +X -Y -Z
    1, 2, 5, // -X +Y -Z
    0, 5, 2, // +X +Y -Z
};
struct GlobalUniforms {
  float local_coords_matrix[4][4];
  float transform_projection_matrix[4][4];
  float color[3] = {1.0f, 1.0f, 1.0f};
};
VkBuffer vk_uniform_buffer;
VmaAllocation vk_uniform_buffer_allocation;
GlobalUniforms *vk_uniform_buffer_memory;
VkDescriptorPool vk_descriptor_pool;
VkDescriptorSet vk_uniform_descriptor_set;


animation::AnimationEngine g_engine;
void SetStarterAnimation(){
    g_engine.AddStep();
    g_engine.AddStep();
    auto &step1 = g_engine.GetStepRef(0);
    step1.p1 = {-5, 5, -20};
    step1.p2 = {-2.5, 0, -17.5};
    step1.p3 = {0, 5, -15};
    step1.rotation_start = {0,0,0};
    step1.rotation_end= {0,90,0};
    auto &step2 = g_engine.GetStepRef(1);
    step2.p1 = {0,5,-15};
    step2.p2 = {2.5,0,-15};
    step2.p3 = {5,5,-10};
    step2.rotation_start = {0,90,0};
    step2.rotation_end= {0,180,0};
}
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
  //Index buffer
  const VkBufferCreateInfo index_buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = indexes.size() * sizeof(uint16_t),
      .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  const VmaAllocationCreateInfo index_buffer_allocation_info{
      .flags = (VMA_ALLOCATION_CREATE_MAPPED_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT),
      .usage = VMA_MEMORY_USAGE_AUTO};
  if (vmaCreateBuffer(context.allocator, &index_buffer_info,
                      &index_buffer_allocation_info, &vk_index_buffer,
                      &vk_index_buffer_allocation, nullptr) != VK_SUCCESS) {
    std::cerr << "Failed to create and allocate a vertex buffer\n";
    return false;
  }
  if (vmaMapMemory(context.allocator, vk_index_buffer_allocation,
                   reinterpret_cast<void **>(&vk_index_buffer_memory)) !=
      VK_SUCCESS) {
    std::cerr << "Failed to map index buffer memory\n";
    return false;
  }
  memcpy(vk_vertex_buffer_memory, vertices.data(),
         vertices.size() * sizeof(graphics::Vertex));
  memcpy(vk_index_buffer_memory, indexes.data(),
         indexes.size() * sizeof(uint16_t));
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
      .poolSizeCount = sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]),
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

  SetStarterAnimation();
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
  vmaUnmapMemory(context.allocator, vk_index_buffer_allocation);
  vmaDestroyBuffer(context.allocator, vk_index_buffer,
      vk_index_buffer_allocation);
  vmaUnmapMemory(context.allocator, vk_uniform_buffer_allocation);
  vmaDestroyBuffer(context.allocator, vk_uniform_buffer, vk_uniform_buffer_allocation);
}
std::vector<bool> states{true, true, true};
double prev_time = 0;
const int PROJECTION_MATRIX_ID = 0;
const int ORTOGRAPHIC_MATRIX_ID = 1;
int g_matrix_to_use = 0;
animation::FigureState g_figure_state{
    .position = {0,0,-20.0f},
   .rotation = {0,0,0},
   .scale = {1,1,1}
};
float g_fov = 70.0f;
float g_animation_speed = 1.0f;
bool g_animation_paused = false;
bool g_is_animation_looped = false;
double g_prev_time = 0;
void update([[maybe_unused]] double time) {
  auto &context = graphics::internal::context;
  double delta_time = time - g_prev_time;
  g_prev_time = time;
  ImGui_update(time);
  glm::mat4 to_2d_matrix;
  if(g_matrix_to_use == PROJECTION_MATRIX_ID){
      to_2d_matrix = glm::perspectiveFov(
          glm::radians(g_fov), static_cast<float>(context.swapchain_extent.width),
          static_cast<float>(context.swapchain_extent.height), 0.1f, 100.0f);
  }else if(g_matrix_to_use == ORTOGRAPHIC_MATRIX_ID){
          const float w = static_cast<float>(context.swapchain_extent.width);
          const float h = static_cast<float>(context.swapchain_extent.height);
          const float aspect = w / h;
          const float half_height = 10.0f;
          const float half_width = half_height * aspect;
      to_2d_matrix = glm::orthoZO(-half_width, half_width, -half_height, half_height, 0.1f, 100.0f);
  }else{
      return;
  }
  if(!g_animation_paused){
      g_engine.Proceed(g_figure_state, g_animation_speed, delta_time, g_is_animation_looped);
  }
  glm::mat4 movement = glm::translate(glm::mat4(1.0f), glm::make_vec3(g_figure_state.position.data()));
  glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::make_vec3(g_figure_state.scale.data()));
  glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(g_figure_state.rotation[0]), glm::vec3(1.0f,0.0f,0.0f));
  rotation = glm::rotate(rotation, glm::radians(g_figure_state.rotation[1]), glm::vec3(0.0f,1.0f,0.0f));
  rotation = glm::rotate(rotation, glm::radians(g_figure_state.rotation[2]), glm::vec3(0.0f,0.0f,1.0f));
  glm::mat4 move_and_projection = to_2d_matrix * movement * scaling;
  glm::mat4 scale_and_rotation = rotation;
  memcpy(vk_uniform_buffer_memory->transform_projection_matrix, glm::value_ptr(move_and_projection),
         sizeof(GlobalUniforms::transform_projection_matrix));
  memcpy(vk_uniform_buffer_memory->local_coords_matrix, glm::value_ptr(scale_and_rotation),
         sizeof(GlobalUniforms::local_coords_matrix));
}
void ImGui_update(double time){
    ImGui::Begin("Awesome engine configuration");
    if(ImGui::CollapsingHeader("2D Projection settings")){
        ImGui::RadioButton("Projection matrix", &g_matrix_to_use, PROJECTION_MATRIX_ID); ImGui::SameLine();
        ImGui::RadioButton("Ortographic matrix", &g_matrix_to_use, ORTOGRAPHIC_MATRIX_ID);
        ImGui::SliderFloat("FOV", &g_fov, 10.0f, 120.0f);
    }
    if(ImGui::CollapsingHeader("Figure parameters")){
        if(!g_engine.IsFinished()){
            ImGui::Text("Currently running an animation");
            ImGui::BeginDisabled();
        }
        ImGui::InputFloat3("Position of the center", g_figure_state.position.data());
        ImGui::InputFloat3("Scaling", g_figure_state.scale.data());
        ImGui::InputFloat3("Euler angle", g_figure_state.rotation.data());
        if(!g_engine.IsFinished()){
            ImGui::EndDisabled();
        }
    }
    if(ImGui::CollapsingHeader("Animations. Bezier")){
        if(ImGui::Button("Add step")){
            g_engine.AddStep();
        }
        for(size_t i = 0; i < g_engine.GetStepsCount(); i++){
            ImGui::Text("Step %d", i);
            auto& step = g_engine.GetStepRef(i);
            ImGui::PushID(i);
            ImGui::InputFloat3("P1", step.p1.data());
            ImGui::InputFloat3("P2", step.p2.data());
            ImGui::InputFloat3("P3", step.p3.data());
            ImGui::Text("Rotation");
            ImGui::InputFloat3("Start rotation", step.rotation_start.data());
            ImGui::InputFloat3("End rotation", step.rotation_end.data());
            if(ImGui::Button("Remove step")){
                if(!g_engine.RemoveStep(i)){
                    std::cerr << "Failed to remove a step";
                }
            }
            ImGui::PopID();
        }
        ImGui::SeparatorText("Animation control");
        ImGui::SliderFloat("Animation speed", &g_animation_speed, 0.1f, 5.0f);
        if(ImGui::Button("Start")){
            g_animation_paused = false;
            g_engine.Start(g_figure_state);
        }
        ImGui::SameLine();
        if(ImGui::Button("Pause")){
            g_animation_paused = !g_animation_paused;
        }
        ImGui::SameLine();
        if(ImGui::Button("Stop")){
            g_engine.Stop();
        }
        ImGui::Checkbox("Is looping", &g_is_animation_looped);
        if(g_animation_paused){
            ImGui::Text("Animation is paused");
        }
    }
    if(ImGui::CollapsingHeader("Color edit")){
        ImGui::ColorEdit3("Figure color", vk_uniform_buffer_memory->color);
    }
    ImGui::End();
}

void render(const graphics::internal::FrameData &fd) {
  auto &context = graphics::internal::context;
  vkResetCommandBuffer(fd.command_buffer, 1);
  VkCommandBufferBeginInfo cmd_begin_info = {};
  cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_begin_info.pNext = nullptr;
  VkClearValue clearValues []{
      {.color = {{0.0f, 0.0f, 0.1f, 1.0f}}},
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
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, &vk_vertex_buffer, &offset);
  vkCmdBindIndexBuffer(fd.command_buffer, vk_index_buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics::triangle_pipeline_layout, 0, 1, &vk_uniform_descriptor_set, 0, nullptr);
  vkCmdDrawIndexed(fd.command_buffer, static_cast<uint32_t>(indexes.size()), 1, 0, 0,0);

  // finalize the render pass
  vkCmdEndRenderPass(fd.command_buffer);
  vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
