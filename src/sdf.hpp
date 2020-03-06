#ifndef TRM_SDF_HPP_
#define TRM_SDF_HPP_

#include "type.hpp"

#include <limits>
#include <map>
#include <memory>

#include "interp.hpp"
#include "material.hpp"

namespace trm {
struct Sdf : std::enable_shared_from_this<Sdf> {
  Sdf();
  Sdf(const std::shared_ptr<Material> &mat);
  Sdf(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b = nullptr);
  Sdf(const std::shared_ptr<Material> &mat, const std::shared_ptr<Sdf> &a,
      const std::shared_ptr<Sdf> &b = nullptr);

  Float operator()(const Vec3 &p) const;
  Vec3 normal(const Vec3 &p,
              const Float &ep = 10 * std::numeric_limits<Float>::epsilon());
  std::shared_ptr<Sdf> translate(const Vec3 &xyz);
  std::shared_ptr<Sdf> rotate(const Float &angle, const Vec3 &axis);
  std::shared_ptr<Sdf> scale(const Vec3 &xyz);

  inline virtual Float dist(const Vec3 &) const = 0;

  Mat4 trans, inv;

  std::shared_ptr<Material> mat;
  std::shared_ptr<trm::Sdf> a, b;
};

#define SDF_GEN(TYPE)                                                          \
  template <typename... Args>                                                  \
  std::shared_ptr<Sdf> sdf##TYPE(const Args &... args) {                       \
    return std::make_shared<TYPE>(args...);                                    \
  }

struct Sphere : Sdf {
  template <typename... Args>
  Sphere(const Float &radius, const Args &... args)
      : Sdf(args...), radius(radius) {}
  inline Float dist(const Vec3 &p) const override { return length(p) - radius; }
  Float radius;
};
struct Box : Sdf {
  template <typename... Args>
  Box(const Vec3 &dim, const Args &... args) : Sdf(args...), dim(dim) {}
  template <typename... Args>
  Box(const Float &x, const Float &y, const Float &z, const Args &... args)
      : Sdf(args...), dim(x, y, z) {}
  inline Float dist(const Vec3 &p) const override {
    Vec3 q = abs(p) - dim;
    return length(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
  }
  Vec3 dim;
};
struct Cylinder : Sdf {
  template <typename... Args>
  Cylinder(const Float &h, const Float &r, const Args &... args)
      : Sdf(args...), height(h), radius(r) {}
  inline Float dist(const Vec3 &p) const override {
    Vec2 d = abs(Vec2(length(p.xz()), p.y)) - Vec2(radius, height);
    return min(max(d.x, d.y), 0.0f) + length(max(d, 0.0f));
  }
  Float height, radius;
};
struct Torus : Sdf {
  template <typename... Args>
  Torus(const Vec2 &t, const Args &... args) : Sdf(args...), torus(t) {}
  template <typename... Args>
  Torus(const Float &a, const Float &b, const Args &... args)
      : Sdf(args...), torus(a, b) {}
  inline Float dist(const Vec3 &p) const override {
    Vec2 q = Vec2(length(p.xz()) - torus.x, p.y);
    return length(q) - torus.y;
  }
  Vec2 torus;
};
struct Plane : Sdf {
  template <typename... Args>
  Plane(const Vec4 &n, const Args &... args)
      : Sdf(args...), norm(normalize(n)) {}
  template <typename... Args>
  Plane(const Float &x, const Float &y, const Float &z, const Float &w,
        const Args &... args)
      : Sdf(args...), norm(normalize(Vec4(x, y, z, w))) {}
  inline Float dist(const Vec3 &p) const override {
    return dot(p, norm.xyz()) - norm.w;
  }
  Vec4 norm;
};
struct Pyramid : Sdf {
  template <typename... Args>
  Pyramid(const Float &h, const Args &... args) : Sdf(args...), height(h) {}
  inline Float dist(const Vec3 &p) const override {
    Float m2 = height * height + 0.25f;
    Vec3 p1 = p;
    p1.x = abs(p1.x);
    p1.z = abs(p1.z);
    if (p1.z > p1.x)
      std::swap(p1.x, p1.z);
    p1.x -= 0.5f;
    p1.z -= 0.5f;

    Vec3 q =
        Vec3(p1.z, height * p1.y - 0.5f * p1.x, height * p1.x + 0.5f * p1.y);

    Float s = max(-q.x, 0.0f);
    Float t = clamp((q.y - 0.5f * p1.z) / (m2 + 0.25f), 0.0f, 1.0f);

    Float a = m2 * (q.x + s) * (q.x + s) + q.y * q.y;
    Float b = m2 * (q.x + 0.5f * t) * (q.x + 0.5f * t) +
              (q.y - m2 * t) * (q.y - m2 * t);

    Float d2 = min(q.y, -q.x * m2 - q.y * 0.5f) > 0.0f ? 0.0f : min(a, b);
    return sqrt((d2 + q.z * q.z) / m2) * sign(max(q.z, -p1.y));
  }
  Float height;
};

struct MengerSponge : Sdf {
  template <typename... Args>
  MengerSponge(const std::size_t i, const Args &... args)
      : Sdf(args...), iterations(i) {}
  inline Float dist(const Vec3 &p) const override {
    Vec3 q = abs(p) - Vec3(1.0f);
    Float d = length(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
    Float s = 1.0f;
    for (std::size_t i = 0; i < this->iterations; ++i) {
      Vec3 a = mod(p * s, 2.0f) - 1.0f;
      s *= 3.0f;
      Vec3 r = abs(1.0f - 3.0f * abs(a));
      Float da = max(r.x, r.y);
      Float db = max(r.y, r.z);
      Float dc = max(r.z, r.x);
      Float c = (min(da, min(db, dc)) - 1.0f) / s;

      d = max(d, c);
    }
    return d;
  }
  std::size_t iterations;
};
struct SerpinskiTetrahedron : Sdf {
  template <typename... Args>
  SerpinskiTetrahedron(const std::size_t i, const Args &... args)
      : Sdf(args...), iterations(i) {}
  inline Float dist(const Vec3 &p) const override {
    Vec3 q = p;
    std::size_t i = 0;
    while (i < this->iterations) {
      if (q.x + q.y < 0) {
        Float tmp = -q.x;
        q.x = -q.y;
        q.y = tmp;
      }
      if (q.x + q.z < 0) {
        Float tmp = -q.x;
        q.x = -q.z;
        q.z = tmp;
      }
      if (q.y + q.z < 0) {
        Float tmp = -q.y;
        q.y = -q.z;
        q.z = tmp;
      }
      q = q * 2.0f - Vec3(1.0f);
      i++;
    }
    return (length(q)) * pow(2.0f, -Float(i));
  }
  std::size_t iterations;
};

struct Elongate : Sdf {
  template <typename... Args>
  Elongate(const std::shared_ptr<Sdf> &a, const Vec3 &h, const Args &... args)
      : Sdf(args..., a), h(h) {}
  inline Float dist(const Vec3 &p) const override {
    Vec3 q = abs(p) - h;
    return (*this->a)(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
  }
  Vec3 h;
};
struct Round : Sdf {
  template <typename... Args>
  Round(const std::shared_ptr<Sdf> &a, const Float &radius,
        const Args &... args)
      : Sdf(args..., a), radius(radius) {}
  inline Float dist(const Vec3 &p) const override {
    return (*this->a)(p)-radius;
  }
  Float radius;
};
struct Onion : Sdf {
  template <typename... Args>
  Onion(const std::shared_ptr<Sdf> &a, const Float &thickness,
        const Args &... args)
      : Sdf(args..., a), thickness(thickness) {}
  inline Float dist(const Vec3 &p) const override {
    return abs((*this->a)(p)) - thickness;
  }
  Float thickness;
};

struct Union : Sdf {
  template <typename... Args>
  Union(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
        const Args &... args)
      : Sdf(args..., a, b) {}
  inline Float dist(const Vec3 &p) const override {
    return min((*this->a)(p), (*this->b)(p));
  }
};
struct Subtraction : Sdf {
  template <typename... Args>
  Subtraction(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
              const Args &... args)
      : Sdf(args..., a, b) {}
  inline Float dist(const Vec3 &p) const override {
    return max(-(*this->a)(p), (*this->b)(p));
  }
};
struct Intersection : Sdf {
  template <typename... Args>
  Intersection(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
               const Args &... args)
      : Sdf(args..., a, b) {}
  inline Float dist(const Vec3 &p) const override {
    return max((*this->a)(p), (*this->b)(p));
  }
};
struct SmoothUnion : Sdf {
  template <typename... Args>
  SmoothUnion(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
              const Float &radius, const Args &... args)
      : Sdf(args..., a, b), radius(radius) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*this->a)(p);
    Float d2 = (*this->b)(p);
    Float h = max(radius - abs(d1 - d2), 0.0f);
    return min(d1, d2) - h * h * 0.25 / radius;
  }
  Float radius;
};
struct SmoothSubtraction : Sdf {
  template <typename... Args>
  SmoothSubtraction(const std::shared_ptr<Sdf> &a,
                    const std::shared_ptr<Sdf> &b, const Float &radius,
                    const Args &... args)
      : Sdf(args..., a, b), radius(radius) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*this->a)(p);
    Float d2 = (*this->b)(p);
    Float h = max(radius - abs(-d1 - d2), 0.0f);
    return max(-d1, d2) + h * h * 0.25f / radius;
  }
  Float radius;
};
struct SmoothIntersection : Sdf {
  template <typename... Args>
  SmoothIntersection(const std::shared_ptr<Sdf> &a,
                     const std::shared_ptr<Sdf> &b, const Float &radius,
                     const Args &... args)
      : Sdf(args..., a, b), radius(radius) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*this->a)(p);
    Float d2 = (*this->b)(p);
    Float h = max(radius - abs(d1 - d2), 0.0f);
    return max(d1, d2) + h * h * 0.25 / radius;
  }
  Float radius;
};

SDF_GEN(Sphere);
SDF_GEN(Box);
SDF_GEN(Cylinder);
SDF_GEN(Torus);
SDF_GEN(Plane);
SDF_GEN(Pyramid);

SDF_GEN(MengerSponge);
SDF_GEN(SerpinskiTetrahedron);

SDF_GEN(Elongate);
SDF_GEN(Round);
SDF_GEN(Onion);

SDF_GEN(Union);
SDF_GEN(Subtraction);
SDF_GEN(Intersection);

SDF_GEN(SmoothUnion);
SDF_GEN(SmoothSubtraction);
SDF_GEN(SmoothIntersection);

} // namespace trm

#endif // TRM_SDF_HPP_
