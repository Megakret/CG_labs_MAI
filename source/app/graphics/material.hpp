#pragma once

#include <array>
namespace graphics {

struct Material {
  std::array<float, 3> albedo = {1.0f, 1.0f, 1.0f};
  float a;
  std::array<float, 3> specular_strength = {0.1,0.1,0.1};
  float shininess = 50;
};
} // namespace graphics
