#ifndef TRM_SETTINGS_HPP_
#define TRM_SETTINGS_HPP_

#include "camera.hpp"
#include "interp.hpp"
#include "sdf.hpp"
#include "type.hpp"

namespace trm {
struct RenderSettings {
  Float maximum_distance = 1e3;
  Float epsilon_distance = 1e-3;
  Float fov = M_PI / 2.0f;
  uvec2 resolution = uvec2(500, 500);
  std::size_t maximum_depth = 16;
  std::size_t spp = 4;
  bool no_bar = false;

  Float inter_pixel_arc = 0.0f;
};
struct Scene {
  struct SceneInstant {
    std::vector<std::shared_ptr<trm::Sdf>> objects;
    trm::Camera::CameraInstant camera;
    Float inter_pixel_arc = 0.0f;
  };
  inline void clear_objs() { objects.clear(); }
  inline void clear_interp() {
    for (auto &obj : objects) {
      obj->clear();
    }
    camera.clear();
  }
  // inline void clear_materials() {
  //
  // }
  // std::vector<std::shared_ptr<trm::Material>> materials;
  std::vector<std::shared_ptr<trm::Sdf>> objects;
  trm::Camera camera;
};
} // namespace trm

#endif // TRM_SETTINGS_HPP_
