#include "sdf.hpp"
#include "type.hpp"

#include <limits>
#include <memory>

trm::Sdf::Sdf() : trans(1.0f), inv(1.0f), mat(nullptr) {}
trm::Sdf::Sdf(const std::shared_ptr<Material> &mat)
    : trans(1.0f), inv(1.0f), mat(mat) {}

void trm::Sdf::construct(const Float &t) {
  trans = glm::translate(glm::mat4_cast(rotation[t]) *
                             glm::scale(Mat4(1.0f), 1.0f / scaling[t]),
                         translation[t]);
  inv = glm::inverse(trans);
  for (auto &var : varf) {
    instantf[var.first] = var.second[t];
  }
  for (auto &var : varf2) {
    instantf2[var.first] = var.second[t];
  }
  for (auto &var : varf3) {
    instantf3[var.first] = var.second[t];
  }
  for (auto &var : varf4) {
    instantf4[var.first] = var.second[t];
  }
}

Float trm::Sdf::operator()(const Vec3 &p) const {
  return this->dist(Vec3(this->inv * Vec4(p, 1.0f)));
}
Vec3 trm::Sdf::normal(const Vec3 &p, const Float &ep) {
  Vec3 op = this->inv * Vec4(p, 1.0f);
  return this->trans *
         normalize(Vec4(this->dist(Vec3(op.x + ep, op.y, op.z)) -
                            this->dist(Vec3(op.x - ep, op.y, op.z)),
                        this->dist(Vec3(op.x, op.y + ep, op.z)) -
                            this->dist(Vec3(op.x, op.y - ep, op.z)),
                        this->dist(Vec3(op.x, op.y, op.z + ep)) -
                            this->dist(Vec3(op.x, op.y, op.z - ep)),
                        0.0f));
}
std::shared_ptr<trm::Sdf> trm::Sdf::translate(const Float &t, const Vec3 &xyz) {
  this->translation.insert(t, xyz);
  return shared_from_this();
}
std::shared_ptr<trm::Sdf> trm::Sdf::rotate(const Float &t, const Float &angle,
                                           const Vec3 &axis) {
  this->rotation.insert(t,
                        glm::quat_cast(glm::rotate(Mat4(1.0f), angle, axis)));
  return shared_from_this();
}
std::shared_ptr<trm::Sdf> trm::Sdf::scale(const Float &t, const Vec3 &xyz) {
  this->scaling.insert(t, xyz);
  return shared_from_this();
}
