#include "application.hpp"

#include <cstring>
#include <imgui.h>
#include <iostream>

#include "../graphics_internal.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vulkan/vulkan_core.h>

#include "animation.hpp"
#include "graphics/figure.hpp"
#include "graphics/graphics.hpp"
namespace application {
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
VkBuffer vk_vertex_buffer;
VmaAllocation vk_vertex_buffer_allocation;
graphics::Vertex *vk_vertex_buffer_memory;

VkBuffer vk_index_buffer;
VmaAllocation vk_index_buffer_allocation;
uint16_t *vk_index_buffer_memory;

graphics::Figure::Shape OCTAHEDRON_SHAPE = {
    .vertices =
        std::vector<graphics::Vertex>{
            {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // 0: +X
            {{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}}, // 1: -X
            {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // 2: +Y
            {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}}, // 3: -Y
            {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},  // 4: +Z
            {{0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 0.0f}}, // 5: -Z
        },
    .indices =
        std::vector<uint16_t>{
            0, 2, 4, // +X +Y +Z
            0, 4, 3, // +X -Y +Z
            1, 3, 4, // -X -Y +Z
            1, 4, 2, // -X +Y +Z
            1, 5, 3, // -X -Y -Z
            0, 3, 5, // +X -Y -Z
            1, 2, 5, // -X +Y -Z
            0, 5, 2, // +X +Y -Z
        }

};
// Regular octahedron: 6 unique vertices at +/-1 on each axis.
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
VkDescriptorPool vk_descriptor_pool;
std::vector<graphics::Figure> g_figures;

animation::AnimationEngine g_engine;
void SetStarterAnimation() {
  g_engine.AddStep();
  g_engine.AddStep();
  auto &step1 = g_engine.GetStepRef(0);
  step1.p1 = {-5, 5, -20};
  step1.p2 = {-2.5, 0, -17.5};
  step1.p3 = {0, 5, -15};
  step1.rotation_start = {0, 0, 0};
  step1.rotation_end = {0, 90, 0};
  auto &step2 = g_engine.GetStepRef(1);
  step2.p1 = {0, 5, -15};
  step2.p2 = {2.5, 0, -15};
  step2.p3 = {5, 5, -10};
  step2.rotation_start = {0, 90, 0};
  step2.rotation_end = {0, 180, 0};
}
bool initialize() {
  auto &context = graphics::internal::context;
  graphics::init_pipelines();
  const VkDescriptorPoolSize descriptor_pool_sizes[]{VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 4}};
  const VkDescriptorPoolCreateInfo descriptor_pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount =
          sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]),
      .pPoolSizes = descriptor_pool_sizes};
  if (vkCreateDescriptorPool(context.device, &descriptor_pool_info, nullptr,
                             &vk_descriptor_pool) != VK_SUCCESS) {
    std::cerr << "Failed to create a descriptor pool\n";
    return false;
  }
  g_figures.emplace_back(OCTAHEDRON_SHAPE, vk_descriptor_pool,
                         graphics::descriptor_set_layout);
  SetStarterAnimation();
  return true;
}

void shutdown() {
  auto &context = graphics::internal::context;
  vkQueueWaitIdle(context.graphics_queue);
  vkDestroyPipeline(context.device, graphics::triangle_pipeline, nullptr);
  vkDestroyPipelineLayout(context.device, graphics::triangle_pipeline_layout,
                          nullptr);
  g_figures.clear();
}
std::vector<bool> states{true, true, true};
double prev_time = 0;
const int PROJECTION_MATRIX_ID = 0;
const int ORTOGRAPHIC_MATRIX_ID = 1;
int g_matrix_to_use = 0;
animation::FigureState g_figure_state{
    .position = {0, 0, -20.0f}, .rotation = {0, 0, 0}, .scale = {1, 1, 1}};
std::array<float, 3> g_color = {1.0f, 1.0f, 1.0f};
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
  glm::mat4 projection_matrix;
  if (g_matrix_to_use == PROJECTION_MATRIX_ID) {
    projection_matrix = glm::perspectiveFov(
        glm::radians(g_fov), static_cast<float>(context.swapchain_extent.width),
        static_cast<float>(context.swapchain_extent.height), 0.1f, 100.0f);
  } else if (g_matrix_to_use == ORTOGRAPHIC_MATRIX_ID) {
    const float w = static_cast<float>(context.swapchain_extent.width);
    const float h = static_cast<float>(context.swapchain_extent.height);
    const float aspect = w / h;
    const float half_height = 10.0f;
    const float half_width = half_height * aspect;
    projection_matrix = glm::orthoZO(-half_width, half_width, -half_height,
                                     half_height, 0.1f, 100.0f);
  } else {
    return;
  }
  if (!g_animation_paused) {
    g_engine.Proceed(g_figure_state, g_animation_speed, delta_time,
                     g_is_animation_looped);
  }
  g_figures[0].UpdateFigure(projection_matrix, g_figure_state.position,
                            g_figure_state.rotation, g_figure_state.scale,
                            g_color);
}
void ImGui_update(double time) {
  ImGui::Begin("Awesome engine configuration");
  if (ImGui::CollapsingHeader("2D Projection settings")) {
    ImGui::RadioButton("Projection matrix", &g_matrix_to_use,
                       PROJECTION_MATRIX_ID);
    ImGui::SameLine();
    ImGui::RadioButton("Ortographic matrix", &g_matrix_to_use,
                       ORTOGRAPHIC_MATRIX_ID);
    ImGui::SliderFloat("FOV", &g_fov, 10.0f, 120.0f);
  }
  if (ImGui::CollapsingHeader("Figure parameters")) {
    if (!g_engine.IsFinished()) {
      ImGui::Text("Currently running an animation");
      ImGui::BeginDisabled();
    }
    ImGui::InputFloat3("Position of the center",
                       g_figure_state.position.data());
    ImGui::InputFloat3("Scaling", g_figure_state.scale.data());
    ImGui::InputFloat3("Euler angle", g_figure_state.rotation.data());
    if (!g_engine.IsFinished()) {
      ImGui::EndDisabled();
    }
  }
  if (ImGui::CollapsingHeader("Animations. Bezier")) {
    if (ImGui::Button("Add step")) {
      g_engine.AddStep();
    }
    for (size_t i = 0; i < g_engine.GetStepsCount(); i++) {
      ImGui::Text("Step %d", i);
      auto &step = g_engine.GetStepRef(i);
      ImGui::PushID(i);
      ImGui::InputFloat3("P1", step.p1.data());
      ImGui::InputFloat3("P2", step.p2.data());
      ImGui::InputFloat3("P3", step.p3.data());
      ImGui::Text("Rotation");
      ImGui::InputFloat3("Start rotation", step.rotation_start.data());
      ImGui::InputFloat3("End rotation", step.rotation_end.data());
      if (ImGui::Button("Remove step")) {
        if (!g_engine.RemoveStep(i)) {
          std::cerr << "Failed to remove a step";
        }
      }
      ImGui::PopID();
    }
    ImGui::SeparatorText("Animation control");
    ImGui::SliderFloat("Animation speed", &g_animation_speed, 0.1f, 5.0f);
    if (ImGui::Button("Start")) {
      g_animation_paused = false;
      g_engine.Start(g_figure_state);
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
      g_animation_paused = !g_animation_paused;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
      g_engine.Stop();
    }
    ImGui::Checkbox("Is looping", &g_is_animation_looped);
    if (g_animation_paused) {
      ImGui::Text("Animation is paused");
    }
  }
  if (ImGui::CollapsingHeader("Color edit")) {
    ImGui::ColorEdit3("Figure color", g_color.data());
  }
  ImGui::End();
}

void render(const graphics::internal::FrameData &fd) {
  auto &context = graphics::internal::context;
  vkResetCommandBuffer(fd.command_buffer, 1);
  VkCommandBufferBeginInfo cmd_begin_info = {};
  cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_begin_info.pNext = nullptr;
  VkClearValue clearValues[]{
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
  g_figures[0].Bind(fd.command_buffer);
  g_figures[0].Draw(fd.command_buffer);

  // finalize the render pass
  vkCmdEndRenderPass(fd.command_buffer);
  vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
