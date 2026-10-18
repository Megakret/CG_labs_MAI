#pragma once

#include <filesystem>

#include "../graphics/figure.hpp"

namespace loaders {
std::vector<graphics::Figure::Shape>
load_models_obj(std::filesystem::path model_path);
}
