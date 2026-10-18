#pragma once

#include <array>

namespace graphics{
    struct SceneLightning {
      std::array<float, 3> ambient = {1, 1, 1};
      float ambient_strength = 0.1f;
    };
}
