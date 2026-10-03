#include <array>
#include <vector>

#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace animation {
//Bezier
struct AnimationStep {
  std::array<float, 3> p1;
  std::array<float, 3> p2;
  std::array<float, 3> p3;
};
struct FigureState {
  std::array<float, 3> position;
  std::array<float, 3> rotation;
  std::array<float, 3> scale;
};
class AnimationEngine {
private:
  std::vector<AnimationStep> steps;
  bool is_animating = false;
  size_t current_step_idx = 0;
  float current_t = 0;
  bool is_forward = true;
public:
  AnimationStep &GetStepRef(size_t i);
  size_t GetStepsCount();
  void AddStep();
  bool RemoveStep(size_t i);
  bool IsFinished();
  bool Proceed(FigureState& current_state, float speed, float dt, bool is_looping);
  void Start(FigureState& state);
  void Stop();
};
} // namespace animation
