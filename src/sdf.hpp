#include "type.hpp"

#include <limits>
#include <memory>

struct Material;

namespace trm {
struct Sdf : std::enable_shared_from_this<Sdf> {
  Sdf();
  Sdf(const std::shared_ptr<Material> &mat);
  Float operator()(const Vec3 &p) const;
  Vec3 normal(const Vec3 &p,
              const Float &ep = 10 * std::numeric_limits<Float>::epsilon());
  std::shared_ptr<Sdf> translate(const Vec3 &xyz);
  std::shared_ptr<Sdf> translate(const Float &x, const Float &y,
                                 const Float &z);
  std::shared_ptr<Sdf> rotate(const Float &angle, const Vec3 &axis);
  std::shared_ptr<Sdf> rotate(const Float &angle, const Float &x,
                              const Float &y, const Float &z);
  std::shared_ptr<Sdf> scale(const Vec3 &xyz);
  std::shared_ptr<Sdf> scale(const Float &x, const Float &y, const Float &z);

  inline virtual Float dist(const Vec3 &) const = 0;

  Mat4 trans, inv;
  std::shared_ptr<Material> mat;
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

struct Elongate : Sdf {
  template <typename... Args>
  Elongate(const Vec3 &h, const std::shared_ptr<Sdf> &a, const Args &... args)
      : Sdf(args...), h(h), a(a) {}
  inline Float dist(const Vec3 &p) const override {
    Vec3 q = abs(p) - h;
    return (*a)(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
  }
  Vec3 h;
  std::shared_ptr<Sdf> a;
};
struct Round : Sdf {
  template <typename... Args>
  Round(const std::shared_ptr<Sdf> &a, const Float &radius,
        const Args &... args)
      : Sdf(args...), radius(radius), a(a) {}
  inline Float dist(const Vec3 &p) const override { return (*a)(p)-radius; }
  Float radius;
  std::shared_ptr<Sdf> a;
};
struct Onion : Sdf {
  template <typename... Args>
  Onion(const Float &thickness, const std::shared_ptr<Sdf> &a,
        const Args &... args)
      : Sdf(args...), thickness(thickness), a(a) {}
  inline Float dist(const Vec3 &p) const override {
    return abs((*a)(p)) - thickness;
  }
  Float thickness;
  std::shared_ptr<Sdf> a;
};

struct Union : Sdf {
  template <typename... Args>
  Union(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
        const Args &... args)
      : Sdf(args...), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    return min((*a)(p), (*b)(p));
  }
  std::shared_ptr<Sdf> a, b;
};
struct Subtraction : Sdf {
  template <typename... Args>
  Subtraction(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
              const Args &... args)
      : Sdf(args...), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    return max(-(*a)(p), (*b)(p));
  }
  std::shared_ptr<Sdf> a, b;
};
struct Intersection : Sdf {
  template <typename... Args>
  Intersection(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
               const Args &... args)
      : Sdf(args...), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    return max((*a)(p), (*b)(p));
  }
  std::shared_ptr<Sdf> a, b;
};
struct SmoothUnion : Sdf {
  template <typename... Args>
  SmoothUnion(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
              const Float &radius, const Args &... args)
      : Sdf(args...), a(a), b(b), radius(radius) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*a)(p);
    Float d2 = (*b)(p);
    Float h = max(radius - abs(d1 - d2), 0.0f);
    return min(d1, d2) - h * h * 0.25 / radius;
  }
  std::shared_ptr<Sdf> a, b;
  Float radius;
};
struct SmoothSubtraction : Sdf {
  template <typename... Args>
  SmoothSubtraction(const std::shared_ptr<Sdf> &a,
                    const std::shared_ptr<Sdf> &b, const Float &radius,
                    const Args &... args)
      : Sdf(args...), a(a), b(b), radius(radius) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*a)(p);
    Float d2 = (*b)(p);
    Float h = max(radius - abs(-d1 - d2), 0.0f);
    return max(-d1, d2) + h * h * 0.25f / radius;
  }
  std::shared_ptr<Sdf> a, b;
  Float radius;
};
struct SmoothIntersection : Sdf {
  template <typename... Args>
  SmoothIntersection(const std::shared_ptr<Sdf> &a,
                     const std::shared_ptr<Sdf> &b, const Float &radius,
                     const Args &... args)
      : Sdf(args...), a(a), b(b), radius(radius) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*a)(p);
    Float d2 = (*b)(p);
    Float h = max(radius - abs(d1 - d2), 0.0f);
    return max(d1, d2) + h * h * 0.25 / radius;
  }
  std::shared_ptr<Sdf> a, b;
  Float radius;
};

SDF_GEN(Sphere);
SDF_GEN(Box);
SDF_GEN(Cylinder);
SDF_GEN(Torus);
SDF_GEN(Plane);

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
