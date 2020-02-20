#include <bits/c++config.h>
#include <cstdio>
#include <iostream>
#include <limits>
#include <memory>
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
#include "sdf.hpp"

// PROF_STREAM_FILE("prof.json");

using namespace glm;

#ifdef DOUBLE_PERC
typedef glm::dvec2 Vec2;
typedef glm::dvec3 Vec3;
typedef glm::dvec4 Vec4;
typedef glm::dmat4 Material4;
typedef double Float;
#else
typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;
typedef glm::mat4 Material4;
typedef float Float;
#endif
namespace trm {
struct Sdf;
}
struct Material;

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
  Ray(const Vec3 &o, const Vec3 &d, const std::shared_ptr<Material> &medium)
      : o(o), d(normalize(d)), medium(medium) {}
  Vec3 o, d;
  std::shared_ptr<Material> medium;
};
struct Camera {
  Vec3 pos, center, up;
};

// Global Variables set from main
static Float maximum_distance = 1e3;
static Float epsilon_distance = 1e-3;
static Float fov = M_PI / 2.0f;
static uvec2 resolution = uvec2(500, 500);
static std::size_t maximum_depth = 16;
static std::size_t spp = 4;
static std::vector<std::shared_ptr<trm::Sdf>> objects;
static Camera camera;
static bool progress_display = true;

static Float inter_pixel_arc = 0.0f;

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

Vec3 hemisphere(const Float &u1, const Float &u2) {
  const Float r = sqrt(1.0 - u1 * u1);
  const Float phi = 2 * M_PI * u2;
  return Vec3(cos(phi) * r, sin(phi) * r, u1);
}

std::tuple<Float, std::shared_ptr<trm::Sdf>> sdfScene(const Vec3 &p) {
  Float dist = std::numeric_limits<Float>::infinity();
  std::shared_ptr<trm::Sdf> closest_obj = nullptr;
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

std::tuple<Float, std::shared_ptr<trm::Sdf>> rayMarch(const Ray &r,
                                                      Float *safe_depth) {
  Float dist = 0.0;
  Float delta_dist = 0.0;
  bool not_safe = false;
  std::shared_ptr<trm::Sdf> obj = nullptr;
  for (dist = 0.0; dist < maximum_distance; dist += delta_dist) {
    std::tie(delta_dist, obj) = sdfScene(r.o + dist * r.d);
    if (!not_safe && safe_depth != nullptr && delta_dist < inter_pixel_arc) {
      *safe_depth = dist;
      not_safe = true;
    }
    if (delta_dist < epsilon_distance) {
      return std::make_tuple(dist, obj);
    }
  }
  return std::make_tuple(dist, nullptr);
}

Vec3 trace(const Ray &r, Float *safe_depth, std::size_t depth = 0) {
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
  std::shared_ptr<trm::Sdf> obj;
  std::tie(t, obj) = rayMarch(r, safe_depth);
  if (obj == nullptr)
    return color;

  Vec3 hp = r.o + r.d * t;
  Vec3 n = obj->normal(hp);

  const Float emission = obj->mat->emission;
  color = emission * obj->mat->color * rr_factor;

  if (obj->mat->type == Material::DIFF) {
    Vec3 rotx, roty;
    ons(n, rotx, roty);
    Vec3 sampled_dir = hemisphere(frand(), frand());
    Vec3 rotated_dir;
    rotated_dir.x = dot(Vec3(rotx.x, roty.x, n.x), sampled_dir);
    rotated_dir.y = dot(Vec3(rotx.y, roty.y, n.y), sampled_dir);
    rotated_dir.z = dot(Vec3(rotx.z, roty.z, n.z), sampled_dir);
    Float cost = dot(rotated_dir, n);
    color += trace({hp + (10.0f * epsilon_distance * rotated_dir), rotated_dir,
                    r.medium},
                   nullptr, depth + 1) *
             obj->mat->color * cost * rr_factor * 0.25f; // Should be 0.1f
  } else if (obj->mat->type == Material::SPEC) {
    Vec3 new_dir = normalize(reflect(r.d, n));
    color +=
        trace({hp + (10.0f * epsilon_distance * new_dir), new_dir, r.medium},
              nullptr, depth + 1) *
        rr_factor;
  } else if (obj->mat->type == Material::REFR) {
    Vec3 new_dir =
        refract(r.d, (r.medium == obj->mat) ? -n : n,
                (r.medium != nullptr) ? (r.medium->ior / obj->mat->ior)
                                      : (1.0f / obj->mat->ior));
    color += trace({hp + (10.0f * epsilon_distance * new_dir), new_dir,
                    (r.medium != nullptr && r.medium == obj->mat) ? nullptr
                                                                  : obj->mat},
                   nullptr, depth + 1) *
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

  Material4 view = inverse(lookAtLH(camera.pos, camera.center, camera.up));
  Float filmz = resolution.x / (2.0f * tan(fov / 2.0f));
  Vec3 origin = view * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
  inter_pixel_arc = sqrt(2.0f) / filmz;
#pragma omp parallel for schedule(dynamic, 256) shared(buffer, bar)
  for (std::size_t i = 0; i < resolution.x * resolution.y; ++i) {
    PROF_SCOPED("pixel", "renderer");
    std::size_t x = i % resolution.x;
    std::size_t y = i / resolution.x;
    vec3 color(0.0f, 0.0f, 0.0f);
    Float safe_depth = 0.0f;
    for (std::size_t s = 0; s < spp; ++s) {
      Ray ray(origin,
              view * Vec4(x - resolution.x / 2.0f + frand(),
                          y - resolution.y / 2.0f + frand(), filmz, 0.0f));
      color += trace(ray, &safe_depth) / Float(spp);
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

  objects.push_back(
      trm::sdfPlane(0.0, 1.0, 0.0, -5.0, matDiff({1.0, 1.0, 1.0})));
  objects.push_back(
      trm::sdfPlane(0.0, -1.0, 0.0, -1.0, matEmis(10.0, {1.0, 1.0, 1.0})));
  objects.push_back(
      trm::sdfPlane(-1.0, 0.0, 0.0, -10.0, matDiff({1.0, 0.2, 0.2})));
  objects.push_back(
      trm::sdfPlane(1.0, 0.0, 0.0, -10.0, matDiff({0.2, 1.0, 0.2})));
  objects.push_back(
      trm::sdfPlane(0.0, 0.0, -1.0, -20.0, matDiff({0.2, 0.2, 1.0})));
  objects.push_back(
      trm::sdfPlane(0.0, 0.0, 1.0, 0.0, matDiff({1.0, 1.0, 1.0})));

  auto white_diff = matDiff({1.0, 1.0, 1.0});
  auto white_spec = matSpec({1.0, 1.0, 1.0});
  auto white_refr = matRefr(1.5, {1.0, 1.0, 1.0});
  auto rand_rot = []() { return normalize(Vec3(frand(), frand(), frand())); };

  objects.push_back(trm::sdfSphere(0.75, white_diff)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(0.0f, -4.0f, 10.0f));
  objects.push_back(trm::sdfBox(0.5, 0.5, 0.5, white_diff)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(-3.0f, -4.0f, 10.0f));
  objects.push_back(trm::sdfCylinder(1.0, 0.2, white_diff)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(3.0f, -4.0f, 10.0f));
  objects.push_back(trm::sdfTorus(1.0, 0.3, white_diff)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(0.0f, -4.0f, 7.0f));
  objects.push_back(trm::sdfRound(trm::sdfBox(0.4, 0.4, 0.4), 0.1, white_diff)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(0.0f, -4.0f, 13.0f));

  objects.push_back(
      trm::sdfSmoothUnion(trm::sdfBox(1.0, 0.5, 1.0),
                          trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0), 0.2,
                          white_diff)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(-3.0f, -4.0f, 7.0f));
  objects.push_back(
      trm::sdfSmoothIntersection(trm::sdfBox(1.0, 0.5, 1.0),
                                 trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
                                 0.2, white_diff)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(3.0f, -4.0f, 7.0f));
  objects.push_back(
      trm::sdfSmoothSubtraction(trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
                                trm::sdfBox(1.0, 0.5, 1.0), 0.2, white_diff)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(-3.0f, -4.0f, 13.0f));

  objects.push_back(trm::sdfSphere(0.75, white_spec)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(3.0f, -4.0f, 13.0f));
  objects.push_back(trm::sdfBox(0.5, 0.5, 0.5, white_spec)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(0.0f, -4.0f, 4.0f));
  objects.push_back(trm::sdfCylinder(1.0, 0.2, white_spec)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(0.0f, -4.0f, 16.0f));
  objects.push_back(trm::sdfTorus(1.0, 0.3, white_spec)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(6.0f, -4.0f, 10.0f));
  objects.push_back(trm::sdfRound(trm::sdfBox(0.4, 0.4, 0.4), 0.1, white_spec)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(-6.0f, -4.0f, 10.0f));

  objects.push_back(
      trm::sdfSmoothUnion(trm::sdfBox(1.0, 0.5, 1.0),
                          trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0), 0.2,
                          white_spec)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(-6.0f, -4.0f, 7.0f));
  objects.push_back(
      trm::sdfSmoothIntersection(trm::sdfBox(1.0, 0.5, 1.0),
                                 trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
                                 0.2, white_spec)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(6.0f, -4.0f, 7.0f));
  objects.push_back(
      trm::sdfSmoothSubtraction(trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
                                trm::sdfBox(1.0, 0.5, 1.0), 0.2, white_spec)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(-6.0f, -4.0f, 13.0f));

  objects.push_back(trm::sdfSphere(0.75, white_refr)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(6.0f, -4.0f, 13.0f));
  objects.push_back(trm::sdfBox(0.5, 0.5, 0.5, white_refr)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(6.0f, -4.0f, 7.0f));
  objects.push_back(trm::sdfCylinder(1.0, 0.2, white_refr)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(3.0f, -4.0f, 4.0f));
  objects.push_back(trm::sdfTorus(1.0, 0.3, white_refr)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(-3.0f, -4.0f, 4.0f));
  objects.push_back(trm::sdfRound(trm::sdfBox(0.4, 0.4, 0.4), 0.1, white_refr)
                        ->rotate(M_PI / 2.0f * frand(), rand_rot())
                        ->translate(3.0f, -4.0f, 16.0f));

  objects.push_back(
      trm::sdfSmoothUnion(trm::sdfBox(1.0, 0.5, 1.0),
                          trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0), 0.2,
                          white_refr)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(-3.0f, -4.0f, 16.0f));
  objects.push_back(
      trm::sdfSmoothIntersection(trm::sdfBox(1.0, 0.5, 1.0),
                                 trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
                                 0.2, white_refr)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(6.0f, -4.0f, 16.0f));
  objects.push_back(
      trm::sdfSmoothSubtraction(trm::sdfSphere(0.5)->translate(0.0, 0.5, 0.0),
                                trm::sdfBox(1.0, 0.5, 1.0), 0.2, white_refr)
          ->rotate(M_PI / 2.0f * frand(), rand_rot())
          ->translate(-6.0f, -4.0f, 4.0f));

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
