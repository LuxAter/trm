#include "sdf.hpp"
#include "type.hpp"

#include <limits>
#include <memory>

trm::Sdf::Sdf()
    : trans(1.0f), inv(1.0f), mat(nullptr) {}
trm::Sdf::Sdf(const std::shared_ptr<Material> &mat)
    : trans(1.0f), inv(1.0f), mat(mat) {}
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
std::shared_ptr<trm::Sdf> trm::Sdf::translate(const Vec3 &xyz) {
  this->trans = glm::translate(this->trans, xyz);
  this->inv = glm::translate(this->inv, -xyz);
  return shared_from_this();
}
std::shared_ptr<trm::Sdf> trm::Sdf::translate(const Float &x, const Float &y,
                                              const Float &z) {
  Vec3 xyz(x, y, z);
  this->trans = glm::translate(this->trans, xyz);
  this->inv = glm::translate(this->inv, -xyz);
  return shared_from_this();
}
std::shared_ptr<trm::Sdf> trm::Sdf::rotate(const Float &angle,
                                           const Vec3 &axis) {
  this->trans = glm::rotate(this->trans, angle, axis);
  this->inv = glm::rotate(this->inv, -angle, axis);
  return shared_from_this();
}
std::shared_ptr<trm::Sdf> trm::Sdf::rotate(const Float &angle, const Float &x,
                                           const Float &y, const Float &z) {
  Vec3 axis(x, y, z);
  this->trans = glm::rotate(this->trans, angle, axis);
  this->inv = glm::rotate(this->inv, -angle, axis);
  return shared_from_this();
}
std::shared_ptr<trm::Sdf> trm::Sdf::scale(const Vec3 &xyz) {
  this->trans = glm::scale(this->trans, xyz);
  this->inv = glm::scale(this->inv, 1.0f / xyz);
  return shared_from_this();
}
std::shared_ptr<trm::Sdf> trm::Sdf::scale(const Float &x, const Float &y,
                                          const Float &z) {
  Vec3 xyz(x, y, z);
  this->trans = glm::scale(this->trans, xyz);
  this->inv = glm::scale(this->inv, 1.0f / xyz);
  return shared_from_this();
}
