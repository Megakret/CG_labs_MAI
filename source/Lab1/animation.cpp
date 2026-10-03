#include "animation.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <cmath>
#include <cstring>
namespace animation {
AnimationStep &AnimationEngine::GetStepRef(size_t i) { return steps[i]; }
size_t AnimationEngine::GetStepsCount() { return steps.size(); }
bool AnimationEngine::IsFinished() { return !is_animating; }
void AnimationEngine::AddStep() { steps.push_back(AnimationStep{}); }
bool AnimationEngine::RemoveStep(size_t i) {
  if (i >= steps.size()) {
    return false;
  }
  steps.erase(steps.begin() + i);
  return true;
}
void AnimationEngine::Start(FigureState &state) {
  is_animating = true;
  if (steps.size() == 0) {
    return;
  }
  current_step_idx = 0;
  is_forward = true;
  current_t = 0;
  memcpy(state.position.data(), steps[0].p1.data(), sizeof(state.position));
}
void AnimationEngine::Stop() {
  is_animating = false;
  current_step_idx = 0;
}
glm::vec3 BezierFunc(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
  return (1 - t) * (1 - t) * p1 + 2 * t * (1 - t) * p2 + t * t * p3;
}
glm::vec3 LerpVec3(const glm::vec3& start, const glm::vec3& finish, float t){
   return start + (finish - start)*t;
}
bool AnimationEngine::Proceed(FigureState &state, float speed, float dt,
                              bool is_looping) {
  if (!is_animating) {
    return false;
  }
  if (current_step_idx >= steps.size()) {
    if (is_looping) {
      is_forward = false;
      current_step_idx = steps.size() - 1;
      current_t = 1;
      return true;
    }
    current_step_idx = 0;
    is_animating = false;
    return false;
  }
  if (is_forward) {
    current_t += dt * speed;
  } else {
    current_t -= dt * speed;
  }
  glm::vec3 new_position;
  glm::vec3 new_rotation;
  if (current_t > 1) {
    new_position = glm::make_vec3<float>(steps[current_step_idx].p3.data());
    new_rotation = glm::make_vec3<float>(steps[current_step_idx].rotation_end.data());
    current_t = 0;
    current_step_idx++;
  } else if (current_t < 0) {
    new_position = glm::make_vec3<float>(steps[current_step_idx].p1.data());
    new_rotation = glm::make_vec3<float>(steps[current_step_idx].rotation_start.data());
    current_t = 1;
    if (current_step_idx == 0) {
      if (is_looping) {
        is_forward = true;
        current_step_idx = 0;
        current_t = 0;
        return true;
      }
      current_step_idx = 0;
      is_animating = false;
      return false;
    }
    current_step_idx--;
  } else {
    glm::vec3 p1 = glm::make_vec3<float>(steps[current_step_idx].p1.data());
    glm::vec3 p2 = glm::make_vec3<float>(steps[current_step_idx].p2.data());
    glm::vec3 p3 = glm::make_vec3<float>(steps[current_step_idx].p3.data());
    new_position = BezierFunc(p1, p2, p3, current_t);
    glm::vec3 rotation_start = glm::make_vec3<float>(steps[current_step_idx].rotation_start.data());
    glm::vec3 rotation_end = glm::make_vec3<float>(steps[current_step_idx].rotation_end.data());
    new_rotation = LerpVec3(rotation_start, rotation_end, current_t);
  }
  memcpy(state.position.data(), glm::value_ptr(new_position),
         sizeof(state.position));
  memcpy(state.rotation.data(), glm::value_ptr(new_rotation),
         sizeof(state.rotation));
  return true;
}
} // namespace animation
