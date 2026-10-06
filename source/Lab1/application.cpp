#include "application.hpp"

#include <cstdio>
#include <cstring>
#include <format>
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
struct AnimationConfig {
  float speed = 1.0f;
  bool is_paused = false;
  bool is_looped = false;
};
struct FigureConfig {
  graphics::FigureState figure_state;
  AnimationConfig animation_config;
};
const int FIGURE_COUNT = 2;
std::vector<animation::AnimationEngine> g_animation_engines(FIGURE_COUNT);
std::vector<FigureConfig> g_figure_configs(FIGURE_COUNT);

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

void SetStarterAnimation() {
for(auto& engine: g_animation_engines){
    engine.AddStep();
    engine.AddStep();
    auto &step1 = engine.GetStepRef(0);
    step1.p1 = {-5, 5, -20};
    step1.p2 = {-2.5, 0, -17.5};
    step1.p3 = {0, 5, -15};
    step1.rotation_start = {0, 0, 0};
    step1.rotation_end = {0, 90, 0};
    auto &step2 = engine.GetStepRef(1);
    step2.p1 = {0, 5, -15};
    step2.p2 = {2.5, 0, -15};
    step2.p3 = {5, 5, -10};
    step2.rotation_start = {0, 90, 0};
    step2.rotation_end = {0, 180, 0};
}
}
bool initialize() {
  auto &context = graphics::internal::context;
  graphics::init_pipelines();
  const VkDescriptorPoolSize descriptor_pool_sizes[]{VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 4}};
  const VkDescriptorPoolCreateInfo descriptor_pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 2,
      .poolSizeCount =
          sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]),
      .pPoolSizes = descriptor_pool_sizes};
  if (vkCreateDescriptorPool(context.device, &descriptor_pool_info, nullptr,
                             &vk_descriptor_pool) != VK_SUCCESS) {
    std::cerr << "Failed to create a descriptor pool\n";
    return false;
  }
  g_figures.reserve(FIGURE_COUNT);
  for(int i = 0; i < FIGURE_COUNT; i++){
      g_figures.emplace_back(OCTAHEDRON_SHAPE, vk_descriptor_pool,
                             graphics::descriptor_set_layout);
  }
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
double prev_time = 0;
const int PROJECTION_MATRIX_ID = 0;
const int ORTOGRAPHIC_MATRIX_ID = 1;
int g_matrix_to_use = 0;
float g_fov = 70.0f;
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
  for (size_t i = 0; i < g_figure_configs.size(); i++) {
    auto &config = g_figure_configs[i];
    if (!config.animation_config.is_paused) {
      g_animation_engines[i].Proceed(config.figure_state, config.animation_config.speed,
                       delta_time, config.animation_config.is_looped);
    }
    g_figures[i].UpdateFigure(projection_matrix, config.figure_state.position,
                              config.figure_state.rotation,
                              config.figure_state.scale,
                              config.figure_state.color);
  }
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
  ImGui::End();
  for (size_t i = 0; i < FIGURE_COUNT; i++) {
    std::string window_name = std::format("Figure config {}", i);
    auto &config = g_figure_configs[i];
    auto &engine = g_animation_engines[i];
    ImGui::Begin(window_name.data());
    if (ImGui::CollapsingHeader("Figure parameters")) {
      if (!engine.IsFinished()) {
        ImGui::Text("Currently running an animation");
        ImGui::BeginDisabled();
      }
      ImGui::InputFloat3("Position of the center",
                         config.figure_state.position.data());
      ImGui::InputFloat3("Scaling", config.figure_state.scale.data());
      ImGui::InputFloat3("Euler angle", config.figure_state.rotation.data());
      if (!engine.IsFinished()) {
        ImGui::EndDisabled();
      }
    }
    if (ImGui::CollapsingHeader("Animations. Bezier")) {
      if (ImGui::Button("Add step")) {
        engine.AddStep();
      }
      for (size_t i = 0; i < engine.GetStepsCount(); i++) {
        ImGui::Text("Step %d", i);
        auto &step = engine.GetStepRef(i);
        ImGui::PushID(i);
        ImGui::InputFloat3("P1", step.p1.data());
        ImGui::InputFloat3("P2", step.p2.data());
        ImGui::InputFloat3("P3", step.p3.data());
        ImGui::Text("Rotation");
        ImGui::InputFloat3("Start rotation", step.rotation_start.data());
        ImGui::InputFloat3("End rotation", step.rotation_end.data());
        if (ImGui::Button("Remove step")) {
          if (!engine.RemoveStep(i)) {
            std::cerr << "Failed to remove a step";
          }
        }
        ImGui::PopID();
      }
      ImGui::SeparatorText("Animation control");
      ImGui::SliderFloat("Animation speed", &config.animation_config.speed,
                         0.1f, 5.0f);
      if (ImGui::Button("Start")) {
        config.animation_config.is_paused = false;
        engine.Start(config.figure_state);
      }
      ImGui::SameLine();
      if (ImGui::Button("Pause")) {
        config.animation_config.is_paused = !config.animation_config.is_paused;
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop")) {
        engine.Stop();
      }
      ImGui::Checkbox("Is looping", &config.animation_config.is_looped);
      if (config.animation_config.is_paused) {
        ImGui::Text("Animation is paused");
      }
    }
    if (ImGui::CollapsingHeader("Color edit")) {
      ImGui::ColorEdit3("Figure color", config.figure_state.color.data());
    }
    ImGui::End();
  }
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
  for (auto &figure : g_figures) {
    figure.Bind(fd.command_buffer);
    figure.Draw(fd.command_buffer);
  }

  // finalize the render pass
  vkCmdEndRenderPass(fd.command_buffer);
  vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
