#pragma once

#include <array>

namespace graphics{
    struct PointLight{
        std::array<float, 3> light_color = {1,1,1};
        float intensity = 0.0f;
        std::array<float, 3> position = {0,0,0};
        float linear_term = 0.22;
        float quadratic_term = 0.2;
        float padding[3] = {0,0,0};
    };
    struct Spotlight{
        std::array<float, 3> light_color = {1,1,1};
        float intensity = 0.0f;
        std::array<float, 3> position = {0,0,0};
        float inner_angle = 0;
        std::array<float, 3> spot_direction = {1,1,1};
        float outer_angle = 0;
        float linear_term = 0.22;
        float quadratic_term = 0.2;
        std::array<float, 2> padding;
    };
    struct SceneLightning {
      Spotlight flashlight;
      std::array<float, 3> view_position;
      float b;
      std::array<float, 3> ambient = {1, 1, 1};
      float ambient_strength = 0.0f;
      std::array<float, 3> sunlight_direction = {0,1,1};
      float sunlight_intensity = 0.0f;
      std::array<float, 3> sunlight_color = {1,1,1};
      int point_light_count = 0;
      std::array<float, 3> a;
      int spotlight_count = 0;
    };
}
