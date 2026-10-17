#pragma once

#include <array>

#include "glm/fwd.hpp"

namespace graphics{
    class Camera{
        public:
            std::array<float, 3> position = {0,0,0};
            std::array<float, 3> rotation = {0,0,0};
        public:
            glm::mat4 GetViewMatrix();
    };
}
