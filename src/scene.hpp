#ifndef TRM_SCENE_HPP_
#define TRM_SCENE_HPP_

#include <memory>
#include <vector>

#include "camera.hpp"
#include "material.hpp"
#include "sdf.hpp"
#include "settings.hpp"

namespace trm {
struct Scene {
  std::vector<std::shared_ptr<trm::Material>> materials;
  std::vector<std::shared_ptr<trm::Sdf>> objects;
  trm::Camera camera;
};

bool load_json(const std::string &file, RenderSettings *settings, Scene *scene);
} // namespace trm

#endif // TRM_SCENE_HPP_
