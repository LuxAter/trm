#include <bits/c++config.h>
#include <cstdio>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <regex>
#include <stdexcept>
#include <string>
#include <tuple>

#define GLM_FORCE_SWIZZLE
#define GLM_LEFT_HANDED

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "bar.hpp"
#include "img.hpp"
#include "interp.hpp"
#include "prof.hpp"

// PROF_STREAM_FILE("prof.json");

using namespace glm;

#ifdef DOUBLE_PERC
typedef glm::dvec2 Vec2;
typedef glm::dvec3 Vec3;
typedef glm::dvec4 Vec4;
typedef glm::dmat4 Mat4;
typedef double Float;
#else
typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;
typedef glm::mat4 Mat4;
typedef float Float;
#endif
struct Sdf;
struct Mat;

std::ostream &operator<<(std::ostream &out, const Vec3 &v) {
  return out << fmt::format("[{}, {}, {}]", v.x, v.y, v.z);
}
std::istream &operator>>(std::istream &in, Vec3 &v) {
  in >> v.x;
  if (in.peek() != ',')
    throw std::logic_error("expected a ','");
  in.ignore(1, ' ');
  in >> v.y;
  if (in.peek() != ',')
    throw std::logic_error("expected a ','");
  in.ignore(1, ' ');
  in >> v.z;
  return in;
}

#include <cxxopts.hpp>

// Ray structure
struct Ray {
  Ray(const Vec3 &o, const Vec3 &d) : o(o), d(normalize(d)), medium(nullptr) {}
  Ray(const Vec3 &o, const Vec3 &d, const std::shared_ptr<Mat> &medium)
      : o(o), d(normalize(d)), medium(medium) {}
  Vec3 o, d;
  std::shared_ptr<Mat> medium;
};
struct Camera {
  Vec3 pos, center, up;
};

// Global Variables set from main
static Float maximum_distance = 1e3;
static Float epsilon_distance = 1e-3;
static Float fov = M_PI / 2.0f;
static uvec2 resolution = uvec2(100, 100);
static std::size_t maximum_depth = 16;
static std::size_t spp = 64;
static std::vector<std::shared_ptr<Sdf>> objects;
static Camera camera;
static bool progress_display = true;

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> idist(0, std::numeric_limits<int>::max());
std::uniform_real_distribution<Float> dist(0.0f, 1.0f);
std::uniform_real_distribution<Float> dist2(-1.0f, 1.0f);
inline int irand() { return idist(gen); }
inline Float frand() { return dist(gen); }
inline Float frand2() { return dist2(gen); }

void ons(const Vec3 &v1, Vec3 &v2, Vec3 &v3) {
  if (abs(v1.x) > abs(v1.y)) {
    Float inv_len = 1.0 / sqrt(v1.x * v1.x + v1.z * v1.z);
    v2 = Vec3(-v1.z * inv_len, 0.0, v1.x * inv_len);
  } else {
    Float inv_len = 1.0 / sqrt(v1.y * v1.y + v1.z * v1.z);
    v2 = Vec3(0.0, v1.z * inv_len, -v1.y * inv_len);
  }
  v3 = cross(v1, v2);
}

struct Mat {
  enum Shading { EMIS, DIFF, SPEC, REFR };
  Mat(const Shading &type, const Vec3 &color, const Float &emission,
      const Float &ior)
      : type(type), color(color), emission(emission), ior(ior) {}
  Shading type;
  Vec3 color;
  Float emission, ior;
};
inline std::shared_ptr<Mat> matEmis(const Float &emission,
                                    const Vec3 &color = Vec3(1.0)) {
  return std::make_shared<Mat>(Mat::EMIS, color, emission, 0.0);
}
inline std::shared_ptr<Mat> matDiff(const Vec3 &color = Vec3(1.0)) {
  return std::make_shared<Mat>(Mat::DIFF, color, 0.0, 0.0);
}
inline std::shared_ptr<Mat> matSpec(const Vec3 &color = Vec3(1.0)) {
  return std::make_shared<Mat>(Mat::SPEC, color, 0.0, 0.0);
}
inline std::shared_ptr<Mat> matRefr(const Float &ior,
                                    const Vec3 &color = Vec3(1.0)) {
  return std::make_shared<Mat>(Mat::REFR, color, 0.0, ior);
}

struct Sdf : std::enable_shared_from_this<Sdf> {
  Sdf(const Mat4 &trans, const Mat4 &inv, const std::shared_ptr<Mat> &mat)
      : trans(trans), inv(inv), mat(mat) {}
  inline Float operator()(const Vec3 &p) const {
    return this->dist(Vec3(inv * Vec4(p, 1.0)));
  }
  inline Vec3 normal(const Vec3 &p,
                     const Float &ep = 10 *
                                       std::numeric_limits<Float>::epsilon()) {
    Vec3 op = inv * Vec4(p, 1.0f);
    return trans * normalize(Vec4(this->dist(Vec3(op.x + ep, op.y, op.z)) -
                                      this->dist(Vec3(op.x - ep, op.y, op.z)),
                                  this->dist(Vec3(op.x, op.y + ep, op.z)) -
                                      this->dist(Vec3(op.x, op.y - ep, op.z)),
                                  this->dist(Vec3(op.x, op.y, op.z + ep)) -
                                      this->dist(Vec3(op.x, op.y, op.z - ep)),
                                  0.0));
  }

  inline std::shared_ptr<Sdf> translate(const Vec3 &xyz) {
    this->trans = glm::translate(this->trans, xyz);
    this->inv = glm::translate(this->inv, -xyz);
    return shared_from_this();
  }
  inline std::shared_ptr<Sdf> translate(const Float &x, const Float &y,
                                        const Float &z) {
    return this->translate({x, y, z});
  }
  inline std::shared_ptr<Sdf> rotate(const Float &angle, const Vec3 &xyz) {
    this->trans = glm::rotate(this->trans, angle, xyz);
    this->inv = glm::rotate(this->inv, -angle, xyz);
    return shared_from_this();
  }
  inline std::shared_ptr<Sdf> rotate(const Float &angle, const Float &x,
                                     const Float &y, const Float &z) {
    return this->rotate(angle, {x, y, z});
  }
  inline std::shared_ptr<Sdf> scale(const Vec3 &xyz) {
    this->trans = glm::scale(this->trans, xyz);
    this->inv = glm::scale(this->inv, 1.0f / xyz);
    return shared_from_this();
  }
  inline std::shared_ptr<Sdf> scale(const Float &x, const Float &y,
                                    const Float &z) {
    return this->scale({x, y, z});
  }

  inline virtual Float dist(const Vec3 &) const {
    return std::numeric_limits<Float>::infinity();
  }
  Mat4 trans, inv;
  std::shared_ptr<Mat> mat;
};
struct Sphere : Sdf {
  Sphere(const Float &radius, const Mat4 &trans, const Mat4 &inv,
         const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), radius(radius) {}
  inline Float dist(const Vec3 &p) const override { return length(p) - radius; }
  Float radius;
};
std::shared_ptr<Sdf> sdfSphere(const Float &radius,
                               const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Sphere>(radius, Mat4(1.0), Mat4(1.0), mat);
}
struct Box : Sdf {
  Box(const Vec3 &dim, const Mat4 &trans, const Mat4 &inv,
      const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), dim(dim) {}
  inline Float dist(const Vec3 &p) const override {
    Vec3 q = abs(p) - dim;
    return length(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
  }
  Vec3 dim;
};
std::shared_ptr<Sdf> sdfBox(const Vec3 &dim,
                            const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Box>(dim, Mat4(1.0), Mat4(1.0), mat);
}
struct Cylinder : Sdf {
  Cylinder(const Float &h, const Float &r, const Mat4 &trans, const Mat4 &inv,
           const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), height(h), radius(r) {}
  inline Float dist(const Vec3 &p) const override {
    Vec2 d = abs(Vec2(length(p.xz()), p.y)) - Vec2(radius, height);
    return min(max(d.x, d.y), 0.0f) + length(max(d, 0.0f));
  }
  Float height, radius;
};
std::shared_ptr<Sdf> sdfCylinder(const Float &h, const Float &r,
                                 const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Cylinder>(h, r, Mat4(1.0), Mat4(1.0), mat);
}
struct Torus : Sdf {
  Torus(const Vec2 &t, const Mat4 &trans, const Mat4 &inv,
        const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), torus(t) {}
  inline Float dist(const Vec3 &p) const override {
    Vec2 q = Vec2(length(p.xz()) - torus.x, p.y);
    return length(q) - torus.y;
  }
  Vec2 torus;
};
std::shared_ptr<Sdf> sdfTorus(const Vec2 &t,
                              const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Torus>(t, Mat4(1.0), Mat4(1.0), mat);
}
struct Plane : Sdf {
  Plane(const Vec4 &n, const Mat4 &trans, const Mat4 &inv,
        const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), norm(normalize(n)) {}
  inline Float dist(const Vec3 &p) const override {
    return dot(p, norm.xyz()) - norm.w;
  }
  Vec4 norm;
};
std::shared_ptr<Sdf> sdfPlane(const Vec4 &norm,
                              const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Plane>(norm, Mat4(1.0), Mat4(1.0), mat);
}

struct Elongate : Sdf {
  Elongate(const Vec3 &h, const std::shared_ptr<Sdf> &a, const Mat4 &trans,
           const Mat4 &inv, const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), h(h), a(a) {}
  inline Float dist(const Vec3 &p) const override {
    Vec3 q = abs(p) - h;
    return (*a)(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
  }
  Vec3 h;
  std::shared_ptr<Sdf> a;
};
std::shared_ptr<Sdf> sdfElongate(const std::shared_ptr<Sdf> &sdf, const Vec3 &h,
                                 const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Elongate>(h, sdf, Mat4(1.0), Mat4(1.0), mat);
}
struct Round : Sdf {
  Round(const Float &r, const std::shared_ptr<Sdf> &a, const Mat4 &trans,
        const Mat4 &inv, const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), radius(r), a(a) {}
  inline Float dist(const Vec3 &p) const override { return (*a)(p)-radius; }
  Float radius;
  std::shared_ptr<Sdf> a;
};
std::shared_ptr<Sdf> sdfRound(const std::shared_ptr<Sdf> &sdf, const Float &r,
                              const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Round>(r, sdf, Mat4(1.0), Mat4(1.0), mat);
}
struct Onion : Sdf {
  Onion(const Float &thickness, const std::shared_ptr<Sdf> &a,
        const Mat4 &trans, const Mat4 &inv, const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), thickness(thickness), a(a) {}
  inline Float dist(const Vec3 &p) const override {
    return abs((*a)(p)) - thickness;
  }
  Float thickness;
  std::shared_ptr<Sdf> a;
};
std::shared_ptr<Sdf> sdfOnion(const std::shared_ptr<Sdf> &sdf,
                              const Float &thickness,
                              const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Onion>(thickness, sdf, Mat4(1.0), Mat4(1.0), mat);
}
struct InfiniteRepeat : Sdf {
  InfiniteRepeat(const Vec3 &period, const std::shared_ptr<Sdf> &a,
                 const Mat4 &trans, const Mat4 &inv,
                 const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), period(period), a(a) {}
  inline Float dist(const Vec3 &p) const override {
    Vec3 q = mod(p + 0.5f * period, period) - 0.5f * period;
    return (*a)(q);
  }
  Vec3 period;
  std::shared_ptr<Sdf> a;
};
std::shared_ptr<Sdf>
sdfInfiniteRepeat(const std::shared_ptr<Sdf> &sdf, const Vec3 &period,
                  const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<InfiniteRepeat>(period, sdf, Mat4(1.0), Mat4(1.0),
                                          mat);
}

struct Union : Sdf {
  Union(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
        const Mat4 &trans, const Mat4 &inv, const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    return min((*a)(p), (*b)(p));
  }
  std::shared_ptr<Sdf> a, b;
};
std::shared_ptr<Sdf> sdfUnion(const std::shared_ptr<Sdf> &a,
                              const std::shared_ptr<Sdf> &b,
                              const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Union>(a, b, Mat4(1.0), Mat4(1.0), mat);
}
struct Subtraction : Sdf {
  Subtraction(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
              const Mat4 &trans, const Mat4 &inv,
              const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    return max(-(*a)(p), (*b)(p));
  }
  std::shared_ptr<Sdf> a, b;
};
std::shared_ptr<Sdf> sdfSubtraction(const std::shared_ptr<Sdf> &a,
                                    const std::shared_ptr<Sdf> &b,
                                    const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Subtraction>(a, b, Mat4(1.0), Mat4(1.0), mat);
}
struct Intersection : Sdf {
  Intersection(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
               const Mat4 &trans, const Mat4 &inv,
               const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    return max((*a)(p), (*b)(p));
  }
  std::shared_ptr<Sdf> a, b;
};
std::shared_ptr<Sdf>
sdfIntersection(const std::shared_ptr<Sdf> &a, const std::shared_ptr<Sdf> &b,
                const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<Intersection>(a, b, Mat4(1.0), Mat4(1.0), mat);
}

struct SmoothUnion : Sdf {
  SmoothUnion(const Float &r, const std::shared_ptr<Sdf> &a,
              const std::shared_ptr<Sdf> &b, const Mat4 &trans, const Mat4 &inv,
              const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), radius(r), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*a)(p);
    Float d2 = (*b)(p);
    Float h = max(radius - abs(d1 - d2), 0.0f);
    return min(d1, d2) - h * h * 0.25 / radius;
  }
  Float radius;
  std::shared_ptr<Sdf> a, b;
};
std::shared_ptr<Sdf> sdfSmoothUnion(const std::shared_ptr<Sdf> &a,
                                    const std::shared_ptr<Sdf> &b,
                                    const Float &radius,
                                    const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<SmoothUnion>(radius, a, b, Mat4(1.0), Mat4(1.0), mat);
}
struct SmoothSubtraction : Sdf {
  SmoothSubtraction(const Float &r, const std::shared_ptr<Sdf> &a,
                    const std::shared_ptr<Sdf> &b, const Mat4 &trans,
                    const Mat4 &inv, const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), radius(r), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*a)(p);
    Float d2 = (*b)(p);
    Float h = max(radius - abs(-d1 - d2), 0.0f);
    return max(-d1, d2) + h * h * 0.25f / radius;
  }
  Float radius;
  std::shared_ptr<Sdf> a, b;
};
std::shared_ptr<Sdf>
sdfSmoothSubtraction(const std::shared_ptr<Sdf> &a,
                     const std::shared_ptr<Sdf> &b, const Float &radius,
                     const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<SmoothSubtraction>(radius, a, b, Mat4(1.0), Mat4(1.0),
                                             mat);
}
struct SmoothIntersection : Sdf {
  SmoothIntersection(const Float &r, const std::shared_ptr<Sdf> &a,
                     const std::shared_ptr<Sdf> &b, const Mat4 &trans,
                     const Mat4 &inv, const std::shared_ptr<Mat> &mat)
      : Sdf(trans, inv, mat), radius(r), a(a), b(b) {}
  inline Float dist(const Vec3 &p) const override {
    Float d1 = (*a)(p);
    Float d2 = (*b)(p);
    Float h = max(radius - abs(d1 - d2), 0.0f);
    return max(d1, d2) + h * h * 0.25 / radius;
  }
  Float radius;
  std::shared_ptr<Sdf> a, b;
};
std::shared_ptr<Sdf>
sdfSmoothIntersection(const std::shared_ptr<Sdf> &a,
                      const std::shared_ptr<Sdf> &b, const Float &radius,
                      const std::shared_ptr<Mat> &mat = nullptr) {
  return std::make_shared<SmoothIntersection>(radius, a, b, Mat4(1.0),
                                              Mat4(1.0), mat);
}

Vec3 hemisphere(const Float &u1, const Float &u2) {
  const Float r = sqrt(1.0 - u1 * u1);
  const Float phi = 2 * M_PI * u2;
  return Vec3(cos(phi) * r, sin(phi) * r, u1);
}

std::tuple<Float, std::shared_ptr<Sdf>> sdfScene(const Vec3 &p) {
  Float dist = std::numeric_limits<Float>::infinity();
  std::shared_ptr<Sdf> closest_obj = nullptr;
  for (auto &obj : objects) {
    if (obj->mat == nullptr)
      continue;
    Float obj_dist = abs((*obj)(p));
    if (obj_dist < dist) {
      dist = obj_dist;
      closest_obj = obj;
    }
  }
  return std::make_tuple(dist, closest_obj);
}

std::tuple<Float, std::shared_ptr<Sdf>> rayMarch(const Ray &r) {
  Float dist = 0.0;
  Float delta_dist = 0.0;
  std::shared_ptr<Sdf> obj = nullptr;
  for (dist = 0.0; dist < maximum_distance; dist += delta_dist) {
    std::tie(delta_dist, obj) = sdfScene(r.o + dist * r.d);
    if (delta_dist < epsilon_distance) {
      return std::make_tuple(dist, obj);
    }
  }
  return std::make_tuple(dist, nullptr);
}

Vec3 trace(const Ray &r, std::size_t depth = 0) {
  Float rr_factor = 1.0;
  Vec3 color(0.0f);
  if (depth >= maximum_depth) {
    const Float rr_stop_prop = 0.1;
    if (frand() <= rr_stop_prop) {
      return color;
    }
    rr_factor = 1.0 / (1.0 - rr_stop_prop);
  }
  Float t;
  std::shared_ptr<Sdf> obj;
  std::tie(t, obj) = rayMarch(r);
  if (obj == nullptr)
    return color;

  Vec3 hp = r.o + r.d * t;
  Vec3 n = obj->normal(hp);

  const Float emission = obj->mat->emission;
  color = emission * obj->mat->color * rr_factor;

  if (obj->mat->type == Mat::DIFF) {
    Vec3 rotx, roty;
    ons(n, rotx, roty);
    Vec3 sampled_dir = hemisphere(frand(), frand());
    Vec3 rotated_dir;
    rotated_dir.x = dot(Vec3(rotx.x, roty.x, n.x), sampled_dir);
    rotated_dir.y = dot(Vec3(rotx.y, roty.y, n.y), sampled_dir);
    rotated_dir.z = dot(Vec3(rotx.z, roty.z, n.z), sampled_dir);
    Float cost = dot(rotated_dir, n);
    color += trace({hp + (10.0f * epsilon_distance * rotated_dir), rotated_dir},
                   depth + 1) *
             obj->mat->color * cost * rr_factor * 0.25f; // Should be 0.1f
  } else if (obj->mat->type == Mat::SPEC) {
    Vec3 new_dir = normalize(reflect(r.d, n));
    color +=
        trace({hp + (10.0f * epsilon_distance * new_dir), new_dir}, depth + 1) *
        rr_factor;
  } else if (obj->mat->type == Mat::REFR) {
    Vec3 new_dir =
        refract(r.d, (r.medium == obj->mat) ? -n : n,
                (r.medium != nullptr) ? (r.medium->ior / obj->mat->ior)
                                      : (1.0f / obj->mat->ior));
    color += trace({hp + (10.0f * epsilon_distance * new_dir), new_dir,
                    (r.medium != nullptr && r.medium == obj->mat) ? nullptr
                                                                  : obj->mat},
                   depth + 1) *
             1.15f * rr_factor;
  }
  return color;
}

void render(const std::size_t &frame, const Float &t_start,
            const Float t_delta) {
  PROF_FUNC("renderer");
  ProgressBar bar(resolution.y * resolution.x,
                  fmt::format("Frame: {:5d} @ {:7.3f}", frame, t_start),
                  progress_display);
  bar.unit_scale = true;
  bar.unit = "px";
  uint8_t *buffer =
      (uint8_t *)malloc(sizeof(uint8_t) * 3 * resolution.x * resolution.y);

  Mat4 view = inverse(lookAtLH(camera.pos, camera.center, camera.up));
  Float filmz = resolution.x / (2.0f * tan(fov / 2.0f));
  Vec3 origin = view * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
#pragma omp parallel for schedule(dynamic, 256) shared(buffer, bar)
  for (std::size_t i = 0; i < resolution.x * resolution.y; ++i) {
    PROF_SCOPED("pixel", "renderer");
    std::size_t x = i % resolution.x;
    std::size_t y = i / resolution.x;
    vec3 color(0.0f, 0.0f, 0.0f);
    for (std::size_t s = 0; s < spp; ++s) {
      Ray ray(origin,
              view * Vec4(x - resolution.x / 2.0f + frand(),
                          y - resolution.y / 2.0f + frand(), filmz, 0.0f));
      color += trace(ray) / Float(spp);
    }
    buffer[(i * 3) + 0] = clamp(color.r, 0.0f, 1.0f) * 255;
    buffer[(i * 3) + 1] = clamp(color.g, 0.0f, 1.0f) * 255;
    buffer[(i * 3) + 2] = clamp(color.b, 0.0f, 1.0f) * 255;
#pragma omp critical
    if (i % 128 == 0)
      bar.update(128);
  }
  write_file(fmt::format("{frame:05d}.png", fmt::arg("frame", frame),
                         fmt::arg("time", t_start)),
             resolution, buffer);
  if (buffer != nullptr)
    free(buffer);
  bar.finish();
}

int main(int argc, char *argv[]) {
  PROF_BEGIN("cxxopts", "main", "argc", argc, "argv",
             std::vector<std::string>(argv + 1, argv + argc));
  cxxopts::Options options("trm", "Tiny Ray Marcher");
  options.show_positional_help();
  options.add_options("Camera")("p,position", "position of the camera",
                                cxxopts::value<Vec3>())(
      "c,center", "center of the cameras focus", cxxopts::value<Vec3>())(
      "u,up", "direction that will be the top of the image",
      cxxopts::value<Vec3>());
  options.add_options()(
      "o,output", "output file path",
      cxxopts::value<std::string>()->default_value("{frame:010}.png"),
      "FILEDESC");
  options.add_options()("h,help", "show this help message")(
      "no-bar", "disables fancy progress bar display");
  auto result = options.parse(argc, argv);
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  if (result.count("no-bar")) {
    progress_display = false;
  }

  camera.pos = Vec3(0.0f, 0.0f, 0.01f);
  camera.up = Vec3(0.0f, 1.0f, 0.0f);
  camera.center = Vec3(0.0f, -5.0f, 10.0f);

  if (result.count("position")) {
    camera.pos = result["position"].as<Vec3>();
  }
  if (result.count("center")) {
    camera.center = result["center"].as<Vec3>();
  }
  if (result.count("up")) {
    camera.up = result["up"].as<Vec3>();
  }

  PROF_END();
  PROF_BEGIN("scene", "main");

  int ncolors = 15;
  Vec3 colors[] = {Vec3(244, 67, 54) / 255.0f,  Vec3(233, 30, 99) / 255.0f,
                   Vec3(156, 39, 176) / 255.0f, Vec3(103, 58, 183) / 255.0f,
                   Vec3(63, 81, 181) / 255.0f,  Vec3(33, 150, 243) / 255.0f,
                   Vec3(3, 169, 244) / 255.0f,  Vec3(0, 188, 212) / 255.0f,
                   Vec3(0, 150, 136) / 255.0f,  Vec3(76, 175, 80) / 255.0f,
                   Vec3(139, 195, 74) / 255.0f, Vec3(205, 220, 57) / 255.0f,
                   Vec3(255, 235, 59) / 255.0f, Vec3(255, 193, 7) / 255.0f,
                   Vec3(255, 152, 0) / 255.0f};

  objects.push_back(sdfPlane({0.0, 1.0, 0.0, -6.0}, matDiff({1.0, 1.0, 1.0})));
  objects.push_back(
      sdfPlane({0.0, -1.0, 0.0, -25.0}, matEmis(0.0, {1.0, 1.0, 1.0})));

  for (int i = -10; i < 10; ++i) {
    for (int j = 0; j < 20; ++j) {
      Float prim = frand();
      Float type = frand();
      std::shared_ptr<Sdf> primative = nullptr;
      if (prim < 0.25) {
        primative = sdfSphere(1.0f);
      } else if (prim < 0.5) {
        primative = sdfBox({0.5f, 0.5f, 0.5f});
      } else if (prim < 0.75) {
        primative = sdfCylinder(1.0, 0.2);
      } else {
        primative = sdfTorus({1.0f, 0.3f});
      }
      if (type < 0.6) {
        primative->mat = matDiff(colors[irand() % ncolors]);
      } else if (type < 0.7) {
        primative->mat = matSpec(colors[irand() % ncolors]);
      } else if (type < 0.8) {
        primative->mat = matRefr(frand() + 1.0f, colors[irand() % ncolors]);
      } else {
        primative->mat =
            matEmis(100.0f * frand() + 50.0f, colors[irand() % ncolors]);
      }
      primative->rotate(M_PI / 2.0f * frand(), {frand(), frand(), frand()});
      primative->translate(3.0f * i, -5.0f, 3.0f * j);
      objects.push_back(primative);
      // objects.push_back(sdfInfiniteRepeat(primative, {60.0f,
      // INFINITY, 60.0f}));
    }
  }

  // objects.push_back(sdfPlane({0.0, 1.0, 0.0, -5.0},
  // matDiff({1.0, 1.0, 1.0}))); objects.push_back(
  //     sdfPlane({0.0, -1.0, 0.0, -1.0}, matEmis(10.0, {1.0, 1.0, 1.0})));
  // objects.push_back(
  //     sdfPlane({-1.0, 0.0, 0.0, -10.0}, matDiff({1.0, 0.2, 0.2})));
  // objects.push_back(sdfPlane({1.0, 0.0, 0.0, -10.0}, matDiff({0.2, 1.0,
  // 0.2}))); objects.push_back(
  //     sdfPlane({0.0, 0.0, -1.0, -20.0}, matDiff({0.2, 0.2, 1.0})));
  // objects.push_back(sdfPlane({0.0, 0.0, 1.0, 0.0},
  // matDiff({1.0, 1.0, 1.0})));

  // auto white_diff = matDiff({1.0, 1.0, 1.0});
  // auto white_spec = matSpec({1.0, 1.0, 1.0});
  // auto white_refr = matRefr(1.5, {1.0, 1.0, 1.0});
  // auto rand_rot = []() { return normalize(Vec3(frand(), frand(), frand()));
  // };
  //
  // objects.push_back(sdfSphere(0.75, white_diff)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(0.0f, -4.0f, 10.0f));
  // objects.push_back(sdfBox({0.5, 0.5, 0.5}, white_diff)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(-3.0f, -4.0f, 10.0f));
  // objects.push_back(sdfCylinder(1.0, 0.2, white_diff)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(3.0f, -4.0f, 10.0f));
  // objects.push_back(sdfTorus({1.0, 0.3}, white_diff)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(0.0f, -4.0f, 7.0f));
  // objects.push_back(sdfRound(sdfBox({0.4, 0.4, 0.4}), 0.1, white_diff)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(0.0f, -4.0f, 13.0f));
  //
  // objects.push_back(sdfSmoothUnion(sdfBox({1.0, 0.5, 1.0}),
  //                                  sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
  //                                  0.2, white_diff)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(-3.0f, -4.0f, 7.0f));
  // objects.push_back(
  //     sdfSmoothIntersection(sdfBox({1.0, 0.5, 1.0}),
  //                           sdfSphere(0.5)->translate(0.0, 0.5, 0.0), 0.2,
  //                           white_diff)
  //         ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //         ->translate(3.0f, -4.0f, 7.0f));
  // objects.push_back(
  //     sdfSmoothSubtraction(sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
  //                          sdfBox({1.0, 0.5, 1.0}), 0.2, white_diff)
  //         ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //         ->translate(-3.0f, -4.0f, 13.0f));
  //
  // objects.push_back(sdfSphere(0.75, white_spec)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(3.0f, -4.0f, 13.0f));
  // objects.push_back(sdfBox({0.5, 0.5, 0.5}, white_spec)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(0.0f, -4.0f, 4.0f));
  // objects.push_back(sdfCylinder(1.0, 0.2, white_spec)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(0.0f, -4.0f, 16.0f));
  // objects.push_back(sdfTorus({1.0, 0.3}, white_spec)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(6.0f, -4.0f, 10.0f));
  // objects.push_back(sdfRound(sdfBox({0.4, 0.4, 0.4}), 0.1, white_spec)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(-6.0f, -4.0f, 10.0f));
  //
  // objects.push_back(sdfSmoothUnion(sdfBox({1.0, 0.5, 1.0}),
  //                                  sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
  //                                  0.2, white_spec)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(-6.0f, -4.0f, 7.0f));
  // objects.push_back(
  //     sdfSmoothIntersection(sdfBox({1.0, 0.5, 1.0}),
  //                           sdfSphere(0.5)->translate(0.0, 0.5, 0.0), 0.2,
  //                           white_spec)
  //         ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //         ->translate(6.0f, -4.0f, 7.0f));
  // objects.push_back(
  //     sdfSmoothSubtraction(sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
  //                          sdfBox({1.0, 0.5, 1.0}), 0.2, white_spec)
  //         ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //         ->translate(-6.0f, -4.0f, 13.0f));
  //
  // objects.push_back(sdfSphere(0.75, white_refr)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(6.0f, -4.0f, 13.0f));
  // objects.push_back(sdfBox({0.5, 0.5, 0.5}, white_refr)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(6.0f, -4.0f, 7.0f));
  // objects.push_back(sdfCylinder(1.0, 0.2, white_refr)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(3.0f, -4.0f, 4.0f));
  // objects.push_back(sdfTorus({1.0, 0.3}, white_refr)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(-3.0f, -4.0f, 4.0f));
  // objects.push_back(sdfRound(sdfBox({0.4, 0.4, 0.4}), 0.1, white_refr)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(3.0f, -4.0f, 16.0f));
  //
  // objects.push_back(sdfSmoothUnion(sdfBox({1.0, 0.5, 1.0}),
  //                                  sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
  //                                  0.2, white_refr)
  //                       ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //                       ->translate(-3.0f, -4.0f, 16.0f));
  // objects.push_back(
  //     sdfSmoothIntersection(sdfBox({1.0, 0.5, 1.0}),
  //                           sdfSphere(0.5)->translate(0.0, 0.5, 0.0), 0.2,
  //                           white_refr)
  //         ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //         ->translate(6.0f, -4.0f, 16.0f));
  // objects.push_back(
  //     sdfSmoothSubtraction(sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
  //                          sdfBox({1.0, 0.5, 1.0}), 0.2, white_refr)
  //         ->rotate(M_PI / 2.0f * frand(), rand_rot())
  //         ->translate(-6.0f, -4.0f, 4.0f));

  PROF_END();
  render(0, 0.0f, 1 / 60.0f);

  // Interpolation<Float, Vec3> interp(hermite<Float, Vec3>);
  // interp.insert(-1.0, Vec3(-9.9f, 0.0f, 10.0f));
  // interp.insert(0.0, Vec3(0.0f, 0.0f, 0.1f));
  // interp.insert(1.0, Vec3(9.9f, 0.0f, 10.0f));
  // interp.insert(2.0, Vec3(0.0f, 0.0f, 19.9f));
  // interp.insert(3.0, Vec3(-9.9f, 0.0f, 10.0f));
  // interp.insert(4.0, Vec3(0.0f, 0.0f, 0.1f));
  // interp.insert(5.0, Vec3(9.9f, 0.0f, 10.0f));
  // std::size_t frame = 0;
  // for (Float i = interp.begin(1); i <= interp.end(-1); i += 0.01) {
  //   camera.pos = interp[i];
  //   render(frame, i, 1 / 60.0);
  //   frame++;
  //   // std::cout << i << "->" << interp[i] << "\n";
  // }

  return 0;
}
