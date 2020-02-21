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

#ifdef _OPENMP
#include <omp.h>
#endif

#include "argparse.hpp"
#include "bar.hpp"
#include "camera.hpp"
#include "img.hpp"
#include "interp.hpp"
#include "material.hpp"
#include "prof.hpp"
#include "rand.hpp"
#include "scene.hpp"
#include "sdf.hpp"
#include "settings.hpp"
#include "type.hpp"

// PROF_STREAM_FILE("prof.json");

using namespace glm;

// Ray structure
struct Ray {
  Ray(const Vec3 &o, const Vec3 &d) : o(o), d(normalize(d)), medium(nullptr) {}
  Ray(const Vec3 &o, const Vec3 &d,
      const std::shared_ptr<trm::Material> &medium)
      : o(o), d(normalize(d)), medium(medium) {}
  Vec3 o, d;
  std::shared_ptr<trm::Material> medium;
};

// Global Variables set from main
static trm::RenderSettings settings;
static trm::Scene scene;

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

Vec3 hemisphere(const Float &u1, const Float &u2) {
  const Float r = sqrt(1.0 - u1 * u1);
  const Float phi = 2 * M_PI * u2;
  return Vec3(cos(phi) * r, sin(phi) * r, u1);
}

std::tuple<Float, std::shared_ptr<trm::Sdf>> sdfScene(const Vec3 &p) {
  Float dist = std::numeric_limits<Float>::infinity();
  std::shared_ptr<trm::Sdf> closest_obj = nullptr;
  for (auto &obj : scene.objects) {
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
  for (dist = 0.0; dist < settings.maximum_distance; dist += delta_dist) {
    std::tie(delta_dist, obj) = sdfScene(r.o + dist * r.d);
    if (!not_safe && safe_depth != nullptr &&
        delta_dist < settings.inter_pixel_arc) {
      *safe_depth = dist;
      not_safe = true;
    }
    if (delta_dist < settings.epsilon_distance) {
      return std::make_tuple(dist, obj);
    }
  }
  return std::make_tuple(dist, nullptr);
}

Vec3 trace(const Ray &r, Float *safe_depth, std::size_t depth = 0) {
  Float rr_factor = 1.0;
  Vec3 color(0.0f);
  if (depth >= settings.maximum_depth) {
    const Float rr_stop_prop = 0.1;
    if (trm::frand() <= rr_stop_prop) {
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

  if (obj->mat->type == trm::Material::DIFF) {
    Vec3 rotx, roty;
    ons(n, rotx, roty);
    Vec3 sampled_dir = hemisphere(trm::frand(), trm::frand());
    Vec3 rotated_dir;
    rotated_dir.x = dot(Vec3(rotx.x, roty.x, n.x), sampled_dir);
    rotated_dir.y = dot(Vec3(rotx.y, roty.y, n.y), sampled_dir);
    rotated_dir.z = dot(Vec3(rotx.z, roty.z, n.z), sampled_dir);
    Float cost = dot(rotated_dir, n);
    color += trace({hp + (10.0f * settings.epsilon_distance * rotated_dir),
                    rotated_dir, r.medium},
                   nullptr, depth + 1) *
             obj->mat->color * cost * rr_factor * 0.25f; // Should be 0.1f
  } else if (obj->mat->type == trm::Material::SPEC) {
    Vec3 new_dir = normalize(reflect(r.d, n));
    color += trace({hp + (10.0f * settings.epsilon_distance * new_dir), new_dir,
                    r.medium},
                   nullptr, depth + 1) *
             rr_factor;
  } else if (obj->mat->type == trm::Material::REFR) {
    Vec3 new_dir =
        refract(r.d, (r.medium == obj->mat) ? -n : n,
                (r.medium != nullptr) ? (r.medium->ior / obj->mat->ior)
                                      : (1.0f / obj->mat->ior));
    color += trace({hp + (10.0f * settings.epsilon_distance * new_dir), new_dir,
                    (r.medium != nullptr && r.medium == obj->mat) ? nullptr
                                                                  : obj->mat},
                   nullptr, depth + 1) *
             1.15f * rr_factor;
  }
  return color;
}

void render(const std::size_t &frame, const Float &t_start) {
  PROF_FUNC("renderer");
  ProgressBar bar(settings.resolution.y * settings.resolution.x, "",
                  !settings.no_bar);
  bar.unit_scale = true;
  bar.unit = "px";
  uint8_t *buffer = (uint8_t *)malloc(
      sizeof(uint8_t) * 3 * settings.resolution.x * settings.resolution.y);

  Mat4 view =
      inverse(lookAtLH(scene.camera.pos, scene.camera.center, scene.camera.up));
  Float filmz = settings.resolution.x / (2.0f * tan(scene.camera.fov / 2.0f));
  Vec3 origin = view * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
  settings.inter_pixel_arc = sqrt(2.0f) / filmz;
#pragma omp parallel for schedule(dynamic, 256) shared(buffer, bar)
  for (std::size_t i = 0; i < settings.resolution.x * settings.resolution.y;
       ++i) {
    PROF_SCOPED("pixel", "renderer");
    std::size_t x = i % settings.resolution.x;
    std::size_t y = i / settings.resolution.x;
    vec3 color(0.0f, 0.0f, 0.0f);
    Float safe_depth = 0.0f;
    for (std::size_t s = 0; s < settings.spp; ++s) {
      Ray ray(origin,
              view * Vec4(x - settings.resolution.x / 2.0f + trm::frand(),
                          y - settings.resolution.y / 2.0f + trm::frand(),
                          filmz, 0.0f));
      color += trace(ray, &safe_depth) / Float(settings.spp);
    }
    buffer[(i * 3) + 0] = clamp(color.r, 0.0f, 1.0f) * 255;
    buffer[(i * 3) + 1] = clamp(color.g, 0.0f, 1.0f) * 255;
    buffer[(i * 3) + 2] = clamp(color.b, 0.0f, 1.0f) * 255;
#pragma omp critical
    if (i % 128 == 0)
      bar.update(128);
  }
  write_file(fmt::format(settings.output_fmt, fmt::arg("frame", frame),
                         fmt::arg("time", t_start)),
             settings.resolution, buffer);
  if (buffer != nullptr)
    free(buffer);
  bar.finish();
}

int main(int argc, char *argv[]) {
  PROF_BEGIN("argparse", "main", "argc", argc, "argv",
             std::vector<std::string>(argv + 1, argv + argc));

  bool show_help = false;
  std::string json_file = "";

  trm::argparse::Parser parser("Tiny Ray Marcher");
  parser.add("-h,--help", "show this help message", &show_help);
  parser.add("-o,--output", "output file path", &settings.output_fmt);
  parser.add("-s,--spp", &settings.spp, "samples per pixel");
  parser.add("--fov", &scene.camera.fov, "field of view");
  parser.add("--max", &settings.maximum_distance,
             "maximum distance for rays to travel");
  parser.add("--min", &settings.epsilon_distance,
             "distance to consider as an intersection");
  parser.add("--depth", &settings.maximum_depth,
             "number of reflections/refractions to compute");
  parser.add("-B,--no-bar", &settings.no_bar, "display fancy progress bar");
  parser.add("-r,--res,--resolution", &settings.resolution,
             "resolution of output image");
  parser.add("SceneJSON", &json_file, "scene specification json");

  parser.parse(argc, argv);

  if (show_help) {
    parser.help();
    return 0;
  } else if (json_file == "") {
    parser.help();
    std::fprintf(stderr, "ERROR: SceneJson file is required\n");
    return 1;
  }
  PROF_END();
  PROF_BEGIN("loadScene", "main");
  if (!trm::load_json(json_file, &settings, &scene)) {
    parser.help();
    std::fprintf(stderr, "ERROR: SceneJson file \"%s\" did not exist\n",
                 json_file.c_str());
    return 1;
  }

  PROF_END();
  PROF_BEGIN("defaultArg", "main");
  if (scene.camera.fov == 0) {
    scene.camera.fov = M_PI / 2.0f;
  }
  if (settings.resolution.x == 0 || settings.resolution.y == 0) {
    settings.resolution = uvec2(500);
  }
  if (settings.maximum_depth == 0) {
    settings.maximum_depth = 16;
  }
  if (settings.spp == 0) {
    settings.spp = 32;
  }
  if (settings.output_fmt == "") {
    settings.output_fmt = "out/{frame:03}.png";
  }
  PROF_END();

  PROF_BEGIN("scene", "main");
  std::printf("Scene JSON: \"%s\"\n", json_file.c_str());
  std::printf("Settings:\n");
  std::printf("  Resolution:    %ux%u\n", settings.resolution.x,
              settings.resolution.y);
  std::printf("  SPP:           %lu\n", settings.spp);
  std::printf("  Depth:         %lu\n", settings.maximum_depth);
  std::printf("  Output Format: \"%s\"\n", settings.output_fmt.c_str());
  std::printf("Scene:\n");
  std::printf("  Camera:\n");
  std::printf("    FOV:      %f\n", scene.camera.fov);
  std::printf("    Position: %f, %f, %f\n", scene.camera.pos.x,
              scene.camera.pos.y, scene.camera.pos.z);
  std::printf("    Center:   %f, %f, %f\n", scene.camera.center.x,
              scene.camera.center.y, scene.camera.center.z);
  std::printf("    Up :      %f, %f, %f\n", scene.camera.up.x,
              scene.camera.up.y, scene.camera.up.z);
  std::printf("  Objects:   %lu\n", scene.objects.size());
  std::printf("  Materials: %lu\n", scene.materials.size());
  render(0, 0.0f);
  PROF_END();

  return 0;
}
