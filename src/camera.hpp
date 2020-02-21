#ifndef TRM_CAMERA_HPP_
#define TRM_CAMERA_HPP_

#include "interp.hpp"
#include "type.hpp"

namespace trm {
struct Camera {
  Vec3 pos, center, up;
  Float fov;
};
} // namespace trm

#endif // TRM_CAMERA_HPP_
