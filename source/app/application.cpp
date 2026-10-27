#include "application.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>

#include "GLFW/glfw3.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <imgui.h>
#include <vulkan/vulkan_core.h>

#include "../graphics_internal.hpp"
#include "animation.hpp"
#include "gameobject.hpp"
#include "glm/trigonometric.hpp"
#include "graphics/camera.hpp"
#include "graphics/descriptors.hpp"
#include "graphics/figure.hpp"
#include "graphics/graphics.hpp"
#include "graphics/lightning.hpp"
#include "graphics/storage_buffer.hpp"
#include "graphics/uniform_buffer.hpp"
#include "loaders/obj_loader.hpp"

namespace application {
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
const uint32_t SCENE_SET_ID = 0;
const uint32_t SCENE_UNIFORM_BINDING = 0;
const std::filesystem::path TEST_OBJ_FILENAME = "models/viking_room.obj";
const graphics::Figure::Shape OCTAHEDRON_SHAPE = {.vertices =
                                                std::vector<graphics::Vertex>{
                                                    {
                                                        {1.0f, 0.0f, 0.0f},
                                                        {1.0f, 0.0f, 0.0f},
                                                    }, // 0: +X
                                                    {
                                                        {-1.0f, 0.0f, 0.0f},
                                                        {0.0f, 1.0f, 1.0f},
                                                    }, // 1: -X
                                                    {
                                                        {0.0f, 1.0f, 0.0f},
                                                        {0.0f, 1.0f, 0.0f},
                                                    }, // 2: +Y
                                                    {
                                                        {0.0f, -1.0f, 0.0f},
                                                        {1.0f, 0.0f, 1.0f},
                                                    }, // 3: -Y
                                                    {
                                                        {0.0f, 0.0f, 1.0f},
                                                        {0.0f, 0.0f, 1.0f},
                                                    }, // 4: +Z
                                                    {
                                                        {0.0f, 0.0f, -1.0f},
                                                        {1.0f, 1.0f, 0.0f},
                                                    }, // 5: -Z
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
std::unique_ptr<graphics::DescriptorPool> g_descriptor_pool;
std::unique_ptr<graphics::DescriptorSet> g_scene_descriptor_set;
std::unique_ptr<GameobjectManager> g_gameobject_manager;
struct SceneBuffer {
  float view_projection_matrix[4][4];
  graphics::SceneLightning lightning;
};
const size_t MAX_POINT_LIGHT_COUNT = 100;
const size_t MAX_SPOTLIGHT_COUNT = 100;
const size_t POINT_LIGHT_BINDING = 1;
const size_t SPOTLIGHT_BINDING = 2;
std::unique_ptr<graphics::UniformBuffer<SceneBuffer>> g_scene_uniform_buffer;
std::unique_ptr<graphics::StorageBuffer<graphics::PointLight>>
    g_point_lights_buffer;
std::vector<graphics::PointLight> g_point_lights;
std::unique_ptr<graphics::StorageBuffer<graphics::Spotlight>>
    g_spotlights_buffer;
std::vector<graphics::Spotlight> g_spotlights;
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
void init_scene();
bool initialize() {
  graphics::init_pipelines();
  const std::vector<VkDescriptorPoolSize> descriptor_pool_sizes{
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                           .descriptorCount = GAMEOBJECT_COUNT + 1},
      VkDescriptorPoolSize{
          .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount =
              static_cast<uint32_t>(graphics::STORAGE_BUFFER_COUNT),
      }};
  g_descriptor_pool = std::make_unique<graphics::DescriptorPool>(
      descriptor_pool_sizes,
      GAMEOBJECT_COUNT + graphics::STORAGE_BUFFER_COUNT + 1);
  graphics::DescriptorSet scene_set =
      g_descriptor_pool->AllocateSet(graphics::g_scene_descriptor_layout);
  // Scene
  g_scene_uniform_buffer =
      std::make_unique<graphics::UniformBuffer<SceneBuffer>>(
          scene_set, SCENE_UNIFORM_BINDING);
  g_point_lights_buffer =
      std::make_unique<graphics::StorageBuffer<graphics::PointLight>>(
          scene_set, MAX_POINT_LIGHT_COUNT, POINT_LIGHT_BINDING);
  g_spotlights_buffer =
      std::make_unique<graphics::StorageBuffer<graphics::Spotlight>>(
          scene_set, MAX_SPOTLIGHT_COUNT, SPOTLIGHT_BINDING);
  scene_set.Update();
  g_scene_descriptor_set =
      std::make_unique<graphics::DescriptorSet>(std::move(scene_set));
  g_gameobject_manager = std::make_unique<GameobjectManager>(
      *g_descriptor_pool, graphics::g_figure_descriptor_layout);
  // Models
  g_gameobject_manager->gameobjects.reserve(GAMEOBJECT_COUNT);
  graphics::Figure::Shape octahedron = OCTAHEDRON_SHAPE;
  graphics::GenerateNormals(octahedron);
  g_gameobject_manager->Instantiate(octahedron, {0, -5, 0});
  // Loading test model
  auto shapes = loaders::load_models_obj(TEST_OBJ_FILENAME);
  for (auto &shape : shapes) {
    g_gameobject_manager->Instantiate(shape, {0, 0, 0}, {90, 0, 0},
                                      {10, 10, 10});
  }
  SetStarterAnimation();
  g_point_lights.reserve(MAX_POINT_LIGHT_COUNT);
  g_spotlights.reserve(MAX_SPOTLIGHT_COUNT);
  init_scene();
  return true;
}
void init_scene() {
  g_point_lights.emplace_back();
  g_point_lights.emplace_back();
  g_spotlights.emplace_back();
  auto &light1 = g_point_lights[0];
  light1.intensity = 0.5f;
  light1.light_color = {250.0 / 255.0, 125.0 / 255.0, 0.0};
  light1.linear_term = 0.22;
  light1.quadratic_term = 0.2;
  light1.position = {-2.45, -6.56, 6.23};

  auto &light2 = g_point_lights[1];
  light2.intensity = 0.5f;
  light2.light_color = {250.0 / 255.0, 125.0 / 255.0, 0.0};
  light2.linear_term = 0.22;
  light2.quadratic_term = 0.2;
  light2.position = {-2.59, -6.23, -5.649};

  g_scene_lightning_config.ambient = {1.0 / 255.0, 5.0 / 255.0, 30.0 / 255.0};
  g_scene_lightning_config.ambient_strength = 0.05;

  auto &gameobjects = g_gameobject_manager->gameobjects;
  gameobjects[0]->position = {0, -5, 0};
  gameobjects[0]->material.albedo = {1, 203.0 / 255.0, 0};
  gameobjects[0]->material.shininess = 50.0f;
  gameobjects[0]->material.specular_strength = {1, 1, 1};
  gameobjects[1]->material.albedo = {216.0 / 255.0, 154.0 / 255.0, 3.0 / 255.0};
  gameobjects[1]->material.specular_strength = {179.0 / 255.0, 179.0 / 255.0,
                                                179.0 / 255.0};
  gameobjects[1]->material.shininess = 3.0;

  g_scene_lightning_config.sunlight_color = {23.0 / 255.0, 224.0 / 255.0,
                                             130.0 / 255.0};
  g_scene_lightning_config.sunlight_direction = {-1, 1, 0};
  g_scene_lightning_config.sunlight_intensity = 0.03;
  g_spotlights[0] = {
      .light_color = {0, 1, 210.0 / 255.0},
      .intensity = 2,
      .position = {-1.64, -4.61, 0},
      .inner_angle = 40,
      .spot_direction = {-1, 1, 0},
      .outer_angle = 50,
  };
  g_scene_lightning_config.flashlight = graphics::Spotlight{
      .light_color = {244.0 / 255.0, 231.0 / 255.0, 19.0 / 255.0},
      .intensity = 7.0,
      .inner_angle = 20,
      .outer_angle = 30,
  };
}
void shutdown() {
  auto &context = graphics::internal::context;
  vkQueueWaitIdle(context.graphics_queue);
  vkDestroyPipeline(context.device, graphics::triangle_pipeline, nullptr);
  vkDestroyPipelineLayout(context.device, graphics::triangle_pipeline_layout,
                          nullptr);
  g_gameobject_manager = nullptr;
  g_scene_uniform_buffer = nullptr;
  g_point_lights_buffer = nullptr;
  g_spotlights_buffer = nullptr;
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
void SpotlightGui() {
  if (ImGui::Button("Add light")) {
    g_spotlights.emplace_back();
  }
  for (size_t i = 0; i < g_spotlights.size(); ++i) {
    graphics::Spotlight &light = g_spotlights[i];
    if (ImGui::CollapsingHeader(std::format("Spotlight {}", i).c_str())) {
      ImGui::PushID(i);
      ImGui::ColorEdit3("Light color", light.light_color.data());
      ImGui::InputFloat("Intensity", &light.intensity);
      ImGui::InputFloat3("Position", light.position.data());
      ImGui::InputFloat3("Direction", light.spot_direction.data());
      ImGui::InputFloat("Inner angle", &light.inner_angle);
      ImGui::InputFloat("Outer angle", &light.outer_angle);
      ImGui::InputFloat("Linear term", &light.linear_term);
      ImGui::InputFloat("Quadratic term", &light.quadratic_term);
      if (ImGui::Button("Remove light")) {
        g_spotlights[i] = graphics::Spotlight{};
        g_spotlights.erase(g_spotlights.begin() + i);
        i = std::min(static_cast<int>(i) - 1, 0);
      }
      ImGui::PopID();
    }
  }
}
void PointLightGui() {

  if (ImGui::Button("Add light")) {
    g_point_lights.emplace_back();
  }
  for (size_t i = 0; i < g_point_lights.size(); ++i) {
    graphics::PointLight &light = g_point_lights[i];
    if (ImGui::CollapsingHeader(std::format("Point light {}", i).c_str())) {
      ImGui::PushID(i);
      ImGui::ColorEdit3("Light color", light.light_color.data());
      ImGui::InputFloat("Intensity", &light.intensity);
      ImGui::InputFloat3("Position", light.position.data());
      ImGui::InputFloat("Linear term", &light.linear_term);
      ImGui::InputFloat("Quadratic term", &light.quadratic_term);
      if (ImGui::Button("Remove light")) {
        g_point_lights[i] = graphics::PointLight{};
        g_point_lights.erase(g_point_lights.begin() + i);
        i = std::min(static_cast<int>(i) - 1, 0);
      }
      ImGui::PopID();
    }
  }
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
    if (ImGui::CollapsingHeader("Flashlight")) {
      auto &light = g_scene_lightning_config.flashlight;
      ImGui::ColorEdit3("Light color", light.light_color.data());
      ImGui::InputFloat("Intensity", &light.intensity);
      ImGui::InputFloat("Inner angle", &light.inner_angle);
      ImGui::InputFloat("Outer angle", &light.outer_angle);
      ImGui::InputFloat("Linear term", &light.linear_term);
      ImGui::InputFloat("Quadratic term", &light.quadratic_term);
    }
    if (ImGui::CollapsingHeader("Sunlight")) {
      ImGui::ColorEdit3("Sunlight color",
                        g_scene_lightning_config.sunlight_color.data());
      ImGui::InputFloat3("Sunlight direction",
                         g_scene_lightning_config.sunlight_direction.data());
      ImGui::InputFloat("Sunlight intensity",
                        &g_scene_lightning_config.sunlight_intensity);
    }
    if (ImGui::CollapsingHeader("Point lights")) {
      PointLightGui();
    }
    if (ImGui::CollapsingHeader("Spotlights")) {
      SpotlightGui();
    }
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
  // Update flashlight
  graphics::Spotlight &flashlight = g_scene_lightning_config.flashlight;
  flashlight.position = g_camera.position;
  memcpy(flashlight.spot_direction.data(),
         glm::value_ptr(g_camera.GetLookDirection()),
         sizeof(flashlight.spot_direction));
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
      {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
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
  g_scene_descriptor_set->Bind(
      fd.command_buffer, graphics::triangle_pipeline_layout, SCENE_SET_ID);
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
  scene_ptr->lightning.point_light_count = g_point_lights.size();
  scene_ptr->lightning.spotlight_count = g_spotlights.size();
  auto &flashlight = scene_ptr->lightning.flashlight;
  flashlight.inner_angle =
      glm::cos(glm::radians(g_scene_lightning_config.flashlight.inner_angle));
  flashlight.outer_angle =
      glm::cos(glm::radians(g_scene_lightning_config.flashlight.outer_angle));
  // Storage buffers
  auto point_lights_buf = g_point_lights_buffer->GetSpan();
  for (size_t i = 0; i < g_point_lights.size(); i++) {
    point_lights_buf[i] = g_point_lights[i];
  }
  auto spotlight_buf = g_spotlights_buffer->GetSpan();
  for (size_t i = 0; i < g_spotlights.size(); i++) {
    spotlight_buf[i] = g_spotlights[i];
    spotlight_buf[i].inner_angle =
        glm::cos(glm::radians(g_spotlights[i].inner_angle));
    spotlight_buf[i].outer_angle =
        glm::cos(glm::radians(g_spotlights[i].outer_angle));
  }
  for (GameobjectPtr gameobject : g_gameobject_manager->gameobjects) {
    auto &figure = gameobject->figure;
    figure.Bind(fd.command_buffer, graphics::triangle_pipeline_layout);
    figure.UpdateFigure(gameobject->position, gameobject->rotation,
                        gameobject->scale, gameobject->color);
    figure.UpdateMaterial(gameobject->material);
    figure.Draw(fd.command_buffer);
  }
  // finalize the render pass
  vkCmdEndRenderPass(fd.command_buffer);
  vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
