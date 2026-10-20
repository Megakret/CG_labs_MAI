#include "application.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <iostream>
#include <memory>

#include "GLFW/glfw3.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vulkan/vulkan_core.h>

#include "../graphics_internal.hpp"
#include "animation.hpp"
#include "gameobject.hpp"
#include "graphics/camera.hpp"
#include "graphics/figure.hpp"
#include "graphics/graphics.hpp"
#include "graphics/lightning.hpp"
#include "graphics/uniform_buffer.hpp"
#include "loaders/obj_loader.hpp"

namespace application {
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
const uint32_t SCENE_SET_ID = 0;
const uint32_t SCENE_BINDING = 0;
const std::filesystem::path TEST_OBJ_FILENAME = "models/viking_room.obj";
graphics::Figure::Shape OCTAHEDRON_SHAPE =
    {.vertices =
         std::vector<graphics::Vertex>{
             {{1.0f, 0.0f, 0.0f},
              {1.0f, 0.0f, 0.0f},
              {1.0f, 0.0f, 0.0f}}, // 0: +X
             {{-1.0f, 0.0f, 0.0f},
              {0.0f, 1.0f, 1.0f},
              {-1.0f, 0.0f, 0.0f}}, // 1: -X
             {{0.0f, 1.0f, 0.0f},
              {0.0f, 1.0f, 0.0f},
              {0.0f, 1.0f, 0.0f}}, // 2: +Y
             {{0.0f, -1.0f, 0.0f},
              {1.0f, 0.0f, 1.0f},
              {0.0f, -1.0f, 0.0f}}, // 3: -Y
             {{0.0f, 0.0f, 1.0f},
              {0.0f, 0.0f, 1.0f},
              {0.0f, 0.0f, 1.0f}}, // 4: +Z
             {{0.0f, 0.0f, -1.0f},
              {1.0f, 1.0f, 0.0f},
              {0.0f, 0.0f, -1.0f}}, // 5: -Z
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
VkDescriptorPool g_vk_descriptor_pool;
std::unique_ptr<GameobjectManager> g_gameobject_manager;
struct SceneBuffer {
  float view_projection_matrix[4][4];
  graphics::SceneLightning lightning;
};
std::unique_ptr<graphics::UniformBuffer<SceneBuffer>> g_scene_uniform_buffer;
graphics::SceneLightning g_scene_lightning_config;
graphics::Camera g_camera;
void SetStarterAnimation() {
  for (auto gameobject : g_gameobject_manager->gameobjects) {
    animation::AnimationEngine &engine = gameobject->animation_engine;
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
const uint32_t GAMEOBJECT_COUNT = 100;
bool initialize() {
  auto &context = graphics::internal::context;
  graphics::init_pipelines();
  const VkDescriptorPoolSize descriptor_pool_sizes[]{
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                           .descriptorCount = GAMEOBJECT_COUNT + 1}};
  const VkDescriptorPoolCreateInfo descriptor_pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = GAMEOBJECT_COUNT + 1,
      .poolSizeCount =
          sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]),
      .pPoolSizes = descriptor_pool_sizes};
  if (vkCreateDescriptorPool(context.device, &descriptor_pool_info, nullptr,
                             &g_vk_descriptor_pool) != VK_SUCCESS) {
    std::cerr << "Failed to create a descriptor pool\n";
    return false;
  }
  // Scene
  g_scene_uniform_buffer =
      std::make_unique<graphics::UniformBuffer<SceneBuffer>>(
          g_vk_descriptor_pool, graphics::g_scene_descriptor_layout,
          SCENE_BINDING);
  g_gameobject_manager = std::make_unique<GameobjectManager>(
      g_vk_descriptor_pool, graphics::g_figure_descriptor_layout);
  // Models
  g_gameobject_manager->gameobjects.reserve(GAMEOBJECT_COUNT);
  g_gameobject_manager->Instantiate(OCTAHEDRON_SHAPE);
  // Loading test model
//  auto shapes = loaders::load_models_obj(TEST_OBJ_FILENAME);
//  for (auto &shape : shapes) {
//    g_gameobject_manager->Instantiate(shape, {0, 0, 0}, {90, 0, 0},
//                                      {10, 10, 10});
//  }
  SetStarterAnimation();
  return true;
}

void shutdown() {
  auto &context = graphics::internal::context;
  vkQueueWaitIdle(context.graphics_queue);
  vkDestroyPipeline(context.device, graphics::triangle_pipeline, nullptr);
  vkDestroyPipelineLayout(context.device, graphics::triangle_pipeline_layout,
                          nullptr);
  g_gameobject_manager = nullptr;
  g_scene_uniform_buffer = nullptr;
}
double prev_time = 0;
const int PERSPECTIVE_MATRIX_ID = 0;
const int ORTOGRAPHIC_MATRIX_ID = 1;
int g_matrix_to_use = 0;
float g_fov = 70.0f;
double g_prev_time = 0;
namespace camera_controls {
bool g_prev_pressed_mbr;
glm::vec2 g_prev_mouse_pos;
float g_sensitivity = 5.0f;
float g_camera_speed = 10.0f;
const float MAX_Y_ROTATION = 80.0f;
}; // namespace camera_controls
glm::mat4 GetProjectionMatrix() {
  auto &context = graphics::internal::context;
  glm::mat4 projection_matrix(1.0f);
  if (g_matrix_to_use == PERSPECTIVE_MATRIX_ID) {
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
  }
  return projection_matrix;
}
void ImGui_update() {
  ImGui::Begin("Scene settings");
  if (ImGui::CollapsingHeader("Projection settings")) {
    ImGui::RadioButton("Perspective", &g_matrix_to_use, PERSPECTIVE_MATRIX_ID);
    ImGui::SameLine();
    ImGui::RadioButton("Ortographic matrix", &g_matrix_to_use,
                       ORTOGRAPHIC_MATRIX_ID);
    ImGui::SliderFloat("FOV", &g_fov, 10.0f, 120.0f);
  }
  if (ImGui::CollapsingHeader("Camera settings")) {

    ImGui::InputFloat3("Position of the center", g_camera.position.data());
    ImGui::SliderFloat("Sensitivity", &camera_controls::g_sensitivity, 0.1f,
                       10.0f);
    ImGui::SliderFloat("Speed", &camera_controls::g_camera_speed, 0.1f, 100.0f);
  }
  if (ImGui::CollapsingHeader("Lightning settings")) {
    ImGui::ColorEdit3("Ambient color", g_scene_lightning_config.ambient.data());
    ImGui::InputFloat("Ambient strength",
                      &g_scene_lightning_config.ambient_strength);
    ImGui::ColorEdit3("Sunlight color", g_scene_lightning_config.sunlight_color.data());
    ImGui::InputFloat3("Sunlight direction",
                       g_scene_lightning_config.sunlight_direction.data());
    ImGui::InputFloat("Sunlight intensity", &g_scene_lightning_config.sunlight_intensity);
  }
  ImGui::End();
  auto &gameobjects = g_gameobject_manager->gameobjects;
  for (size_t i = 0; i < gameobjects.size(); i++) {
    std::string window_name = std::format("Figure config {}", i);
    GameobjectPtr gameobject = gameobjects[i];
    ImGui::Begin(window_name.data());
    if (ImGui::CollapsingHeader("Figure parameters")) {
      if (!gameobject->animation_engine.IsFinished()) {
        ImGui::Text("Currently running an animation");
        ImGui::BeginDisabled();
      }
      ImGui::InputFloat3("Position of the center", gameobject->position.data());
      ImGui::InputFloat3("Scaling", gameobject->scale.data());
      ImGui::InputFloat3("Euler angle", gameobject->rotation.data());
      if (!gameobject->animation_engine.IsFinished()) {
        ImGui::EndDisabled();
      }
    }
    if (ImGui::CollapsingHeader("Material")) {
      auto &material = gameobject->material;
      ImGui::ColorEdit3("Albedo", material.albedo.data());
      ImGui::ColorEdit3("Specular strength", material.specular_strength.data());
      ImGui::InputFloat("Shininess", &material.shininess);
    }
    if (ImGui::CollapsingHeader("Animations. Bezier")) {
      if (ImGui::Button("Add step")) {
        gameobject->animation_engine.AddStep();
      }
      for (size_t i = 0; i < gameobject->animation_engine.GetStepsCount();
           i++) {
        ImGui::Text("Step %d", static_cast<int>(i));
        auto &step = gameobject->animation_engine.GetStepRef(i);
        ImGui::PushID(i);
        ImGui::InputFloat3("P1", step.p1.data());
        ImGui::InputFloat3("P2", step.p2.data());
        ImGui::InputFloat3("P3", step.p3.data());
        ImGui::Text("Rotation");
        ImGui::InputFloat3("Start rotation", step.rotation_start.data());
        ImGui::InputFloat3("End rotation", step.rotation_end.data());
        if (ImGui::Button("Remove step")) {
          if (!gameobject->animation_engine.RemoveStep(i)) {
            std::cerr << "Failed to remove a step";
          }
        }
        ImGui::PopID();
      }
      ImGui::SeparatorText("Animation control");
      ImGui::SliderFloat("Animation speed", &gameobject->animation_config.speed,
                         0.1f, 5.0f);
      if (ImGui::Button("Start")) {
        gameobject->animation_config.is_paused = false;
        graphics::FigureState start_state = {
            .position = gameobject->position,
            .rotation = gameobject->rotation,
            .scale = gameobject->scale,
        };
        gameobject->animation_engine.Start(start_state);
      }
      ImGui::SameLine();
      if (ImGui::Button("Pause")) {
        gameobject->animation_config.is_paused =
            !gameobject->animation_config.is_paused;
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop")) {
        gameobject->animation_engine.Stop();
      }
      ImGui::Checkbox("Is looping", &gameobject->animation_config.is_looped);
      if (gameobject->animation_config.is_paused) {
        ImGui::Text("Animation is paused");
      }
    }
    if (ImGui::CollapsingHeader("Color edit")) {
      ImGui::ColorEdit3("Figure color", gameobject->color.data());
    }
    ImGui::End();
  }
}
void update([[maybe_unused]] double time, GLFWwindow *window) {
  double delta_time = time - g_prev_time;
  g_prev_time = time;
  ImGui_update();
  // Camera input
  // Rotation
  double mx, my;
  glfwGetCursorPos(window, &mx, &my);
  glm::vec2 cur_cursor_pos(mx, my);
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
    if (camera_controls::g_prev_pressed_mbr) {
      glm::vec2 drag_direction =
          cur_cursor_pos - camera_controls::g_prev_mouse_pos;
      camera_controls::g_prev_mouse_pos = cur_cursor_pos;
      drag_direction *= camera_controls::g_sensitivity * delta_time;
      g_camera.rotation[1] += drag_direction.x;
      g_camera.rotation[0] -= drag_direction.y;
    }
    camera_controls::g_prev_pressed_mbr = true;
  } else {
    camera_controls::g_prev_pressed_mbr = false;
    camera_controls::g_prev_mouse_pos = cur_cursor_pos;
  }
  if (glm::abs(g_camera.rotation[0]) > camera_controls::MAX_Y_ROTATION) {
    g_camera.rotation[0] =
        glm::sign(g_camera.rotation[0]) * camera_controls::MAX_Y_ROTATION;
  }
  // Movement
  glm::vec3 look_direction = g_camera.GetLookDirection();
  glm::vec3 camera_position = glm::make_vec3(g_camera.position.data());
  glm::vec3 move_vector(0.0f, 0.0f, 0.0f);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    move_vector += look_direction;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    move_vector -= look_direction;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    move_vector -=
        glm::normalize(glm::cross(look_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    move_vector +=
        glm::normalize(glm::cross(look_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
  }
  if (move_vector != glm::vec3(0)) {
    move_vector = glm::normalize(move_vector);
  }
  camera_position += move_vector * camera_controls::g_camera_speed *
                     static_cast<float>(delta_time);
  memcpy(g_camera.position.data(), glm::value_ptr(camera_position),
         sizeof(g_camera.position));
  // Figure animation
  auto &gameobjects = g_gameobject_manager->gameobjects;
  for (size_t i = 0; i < gameobjects.size(); i++) {
    GameobjectPtr gameobject = gameobjects[i];
    auto &anim_config = gameobject->animation_config;
    if (!anim_config.is_paused) {
      graphics::FigureState new_state = gameobject->animation_engine.Proceed(
          gameobject->GetFigureState(), anim_config.speed, delta_time,
          anim_config.is_looped);
      gameobject->position = new_state.position;
      gameobject->rotation = new_state.rotation;
      gameobject->scale = new_state.scale;
      gameobject->color = new_state.color;
    }
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
  g_scene_uniform_buffer->Bind(
      fd.command_buffer, graphics::triangle_pipeline_layout, SCENE_SET_ID);
  for (GameobjectPtr gameobject : g_gameobject_manager->gameobjects) {
    auto &figure = gameobject->figure;
    figure.Bind(fd.command_buffer, graphics::triangle_pipeline_layout);
    figure.Draw(fd.command_buffer);
    figure.UpdateFigure(gameobject->position, gameobject->rotation,
                        gameobject->scale, gameobject->color);
    figure.UpdateMaterial(gameobject->material);
  }
  glm::mat4 projection_matrix = GetProjectionMatrix();
  glm::mat4 view_matrix = g_camera.GetViewMatrix();
  glm::mat4 projection_view_matrix = projection_matrix * view_matrix;
  SceneBuffer *scene_ptr = g_scene_uniform_buffer->GetPtr();
  memcpy(scene_ptr->view_projection_matrix,
         glm::value_ptr(projection_view_matrix),
         sizeof(scene_ptr->view_projection_matrix));
  memcpy(&(scene_ptr->lightning), &g_scene_lightning_config,
         sizeof(g_scene_lightning_config));
  scene_ptr->lightning.view_position = g_camera.position;
  // finalize the render pass
  vkCmdEndRenderPass(fd.command_buffer);
  vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
