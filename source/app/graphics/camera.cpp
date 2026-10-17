#include "camera.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"

namespace graphics {
glm::mat4 Camera::GetViewMatrix() {
  glm::mat4 move_matrix =
      glm::translate(glm::mat4(1.0f), glm::make_vec3(position.data()) * (-1.0f));
  glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation[0]),
                           glm::vec3(1.0f, 0.0f, 0.0f));
  rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation[1]),
                           glm::vec3(0.0f, 1.0f, 0.0f));
  rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation[2]),
                           glm::vec3(0.0f, 0.0f, 1.0f));
  rotation_matrix = glm::transpose(rotation_matrix);
  return move_matrix * rotation_matrix;
}
} // namespace graphics
