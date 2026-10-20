#pragma once

#include <array>

namespace graphics{
    struct SceneLightning {
      std::array<float, 3> ambient = {1, 1, 1};
      float ambient_strength = 0.1f;
      std::array<float, 3> sunlight_direction = {0,1,1};
      float sunlight_intensity = 0.5f;
      std::array<float, 3> view_position;
    };
}
