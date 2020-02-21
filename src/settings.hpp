#ifndef TRM_SETTINGS_HPP_
#define TRM_SETTINGS_HPP_

#include "camera.hpp"
#include "interp.hpp"
#include "material.hpp"
#include "sdf.hpp"
#include "type.hpp"

namespace trm {
struct RenderSettings {
  Float maximum_distance = 1e3;
  Float epsilon_distance = 1e-3;
  uvec2 resolution = uvec2(0);
  std::size_t maximum_depth = 0;
  std::size_t spp = 0;
  bool no_bar = false;
  std::string output_fmt = "";

  Float inter_pixel_arc = 0.0f;
};
} // namespace trm

#endif // TRM_SETTINGS_HPP_
