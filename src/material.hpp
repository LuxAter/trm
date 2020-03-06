#ifndef TRM_MATERIAL_HPP_
#define TRM_MATERIAL_HPP_

#include "type.hpp"
#include <memory>

namespace trm {
struct Material {
  enum Shading { EMIS, DIFF, SPEC, REFR };
  Material(const Shading &type, const Vec3 &color, const Float &emission,
           const Float &ior)
      : type(type), color(color), emission(emission), ior(ior) {}
  Shading type;
  Vec3 color;
  Float emission, ior;
};
inline std::shared_ptr<Material> matEmis(const Float &emission,
                                         const Vec3 &color = Vec3(1.0)) {
  return std::make_shared<Material>(Material::EMIS, color, emission, 0.0);
}
inline std::shared_ptr<Material> matDiff(const Vec3 &color = Vec3(1.0)) {
  return std::make_shared<Material>(Material::DIFF, color, 0.0, 0.0);
}
inline std::shared_ptr<Material> matSpec(const Vec3 &color = Vec3(1.0)) {
  return std::make_shared<Material>(Material::SPEC, color, 0.0, 0.0);
}
inline std::shared_ptr<Material> matRefr(const Float &ior,
                                         const Vec3 &color = Vec3(1.0)) {
  return std::make_shared<Material>(Material::REFR, color, 0.0, ior);
}
} // namespace trm

#endif // TRM_MATERIAL_HPP_
