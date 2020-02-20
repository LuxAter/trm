#ifndef TRM_CAMERA_HPP_
#define TRM_CAMERA_HPP_

#include "interp.hpp"
#include "type.hpp"

namespace trm {
struct Camera {
  struct CameraInstant {
    Vec3 pos, center, up;
    Float fov;
  };
  Camera() {}
  Camera(const Vec3 &pos_init, const Vec3 &center_init = Vec3(0.0f, 0.0f, 1.0f),
         const Vec3 &up_init = Vec3(0.0f, 1.0f, 0.0f),
         const Float &fov_init = M_PI / 2.0f) {
    pos.insert(0.0f, pos_init);
    center.insert(0.0f, center_init);
    up.insert(0.0f, up_init);
    fov.insert(0.0f, fov_init);
  }

  inline void clear() {
    pos.clear();
    center.clear();
    up.clear();
    fov.clear();
  }
  inline void insert(const Float &t, const Vec3 &p, const Vec3 &c,
                     const Vec3 &u, const Float &f) {
    pos.insert(t, p);
    center.insert(t, c);
    up.insert(t, u);
    fov.insert(t, f);
  }

  inline Float begin(int offset = 0) const {
    return std::min({pos.begin(offset), center.begin(offset), up.begin(offset),
                     fov.begin()});
  }
  inline Float end(int offset = 0) const {
    return std::max(
        {pos.end(offset), center.end(offset), up.end(offset), fov.end()});
  }
  inline const CameraInstant operator[](const Float &t) const {
    return CameraInstant{pos[t], center[t], up[t], fov[t]};
  }
  Interpolation<Float, Vec3> pos =
      Interpolation<Float, Vec3>(interp::hermite<Float, Vec3>);
  Interpolation<Float, Vec3> center =
      Interpolation<Float, Vec3>(interp::hermite<Float, Vec3>);
  Interpolation<Float, Vec3> up =
      Interpolation<Float, Vec3>(interp::hermite<Float, Vec3>);
  Interpolation<Float, Float> fov =
      Interpolation<Float, Float>(interp::hermite<Float, Float>);
};
} // namespace trm

#endif // TRM_CAMERA_HPP_
