#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <tuple>

#define GLM_FORCE_SWIZZLE
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <cxxopts.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image_write.h"

using namespace glm;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);
inline float frand() { return dist(gen); }

std::tuple<vec3, vec3> inline make_ortho_coord_sys(const vec3 &v1) {
  vec3 v2, v3;
  if (abs(v1.x) > abs(v1.y))
    v2 = vec3(-v1.z, 0.0f, v1.x) * (1.0f / sqrt(v1.x * v1.x + v1.z * v1.z));
  else
    v2 = vec3(0.0f, v1.z, -v1.y) * (1.0f / sqrt(v1.y * v1.y + v1.z * v1.z));
  v3 = cross(v1, v2);
  return std::make_tuple(v2, v3);
}

struct Material {
  std::optional<vec3> sample(const std::shared_ptr<Material> &medium,
                             const vec3 &lo, const vec3 &n) {
    if (type == DIFFUSE) {
      auto [rotx, roty] = make_ortho_coord_sys(n);
      const float u1 = frand(), u2 = frand();
      const float r = sqrt(1.0 - u1 * u1);
      const float phi = 2.0f * M_PI * u2;
      vec3 sampled_dir(cos(phi) * r, sin(phi) * r, u1);
      return vec3(dot(vec3(rotx.x, roty.x, n.x), sampled_dir),
                  dot(vec3(rotx.y, roty.y, n.y), sampled_dir),
                  dot(vec3(rotx.z, roty.z, n.z), sampled_dir));
    } else if (type == EMISSIVE) {
      return std::optional<vec3>();
    } else if (type == SPECULAR) {
      return reflect(lo, n);
    } else if (type == TRANSMISSIVE) {
      return refract(lo, n,
                     medium != nullptr ? (medium->ior / ior) : (1.0f / ior));
    }
    return std::optional<vec3>();
  }

  vec3 evaluate(const vec3 &ri, const vec3 &li, const vec3 &lo, const vec3 &n) {
    if (type == DIFFUSE) {
      return emission * color + 2.0f * ri * color * dot(li, n);
    } else if (type == SPECULAR) {
      return emission * color + ri;
    } else if (type == TRANSMISSIVE) {
      return emission * color + 1.15f * ri;
    } else if (type == EMISSIVE) {
      return emission * color;
    }
    return vec3(0.0f);
  }

  enum { DIFFUSE, EMISSIVE, SPECULAR, TRANSMISSIVE } type;
  vec3 color;
  float ior;
  float emission;
};
class Sdf {
public:
  Sdf() : trans(1.0f), inv(1.0f) {}
  inline float operator()(const vec3 &p) {
    return this->dist(vec3(this->inv * vec4(p, 1.0f)));
  }
  virtual float dist(const vec3 &p, const float &t = 0) = 0;

  inline void translate(const vec3 &xyz) {
    trans = glm::translate(trans, xyz);
    inv = glm::translate(inv, -xyz);
  }

  mat4 trans, inv;
  std::shared_ptr<Material> mat;
};

class Sphere : public Sdf {
public:
  Sphere(const float &r) : Sdf(), radius(r) {}
  virtual float dist(const vec3 &p, const float &t = 0) override {
    return length(p) - radius;
  }

  float radius;
};

struct Global {
  float max_distance = 1e3;
  float epsilon = 1e-5;
  std::size_t bounces = 10;
  std::size_t samples = 16;
  vec3 sky;
  mat4 view;
  float fov = M_PI / 4.0f;
  uvec2 res = uvec2(500, 500);
  vec3 camera_pos = vec3(0, 0, 0), camera_center = vec3(0, 0, -5),
       camera_up = vec3(0, 1, 0);
};

static Global global;

void write_file(const std::string &file_desc, const float *pixels) {
  uint8_t *raw =
      (uint8_t *)malloc(global.res.x * global.res.y * 3 * sizeof(uint8_t));
  for (std::size_t i = 0; i < global.res.x * global.res.y; ++i) {
    raw[i] = pixels[i] * 255;
  }
  if (file_desc.ends_with(".png")) {
    stbi_write_png(file_desc.c_str(), global.res.x, global.res.y, 3, raw,
                   global.res.x * 3 * sizeof(uint8_t));
  } else if (file_desc.ends_with(".bmp")) {
    stbi_write_bmp(file_desc.c_str(), global.res.x, global.res.y, 3, raw);
  } else if (file_desc.ends_with(".tga")) {
    stbi_write_tga(file_desc.c_str(), global.res.x, global.res.y, 3, raw);
  } else if (file_desc.ends_with(".jpg")) {
    stbi_write_jpg(file_desc.c_str(), global.res.x, global.res.y, 3, raw, 75);
  } else if (file_desc.ends_with(".hdf")) {
    stbi_write_hdr(file_desc.c_str(), global.res.x, global.res.y, 3, pixels);
  }
  if (raw != nullptr)
    free(raw);
}

std::tuple<float, std::shared_ptr<Material>> sdfScene(const vec3 &pos) {
  std::shared_ptr<Material> mat = nullptr;
  float dist = std::numeric_limits<float>::infinity();
  float dobj;

  Sphere sph(1.0);
  sph.translate({1.0, 2.0, -10.0});

  dobj = sph(pos);
  if (dobj < dist) {
    dist = dobj;
    mat = sph.mat;
  }

  // Execute sdfs here

  return std::make_tuple(dist, mat);
}

std::tuple<vec3, vec3, std::shared_ptr<Material>> rayMarch(const vec3 &origin,
                                                           const vec3 &dir) {
  vec3 hit_pos;
  float ddist;
  std::shared_ptr<Material> mat = nullptr;
  for (float dist = 0; dist < global.max_distance; dist += ddist) {
    hit_pos = origin + dir * dist;
    std::tie(ddist, mat) = sdfScene(hit_pos);
    if (ddist < global.epsilon) {
      return std::make_tuple(
          hit_pos,
          normalize(
              vec3(std::get<0>(sdfScene(hit_pos + vec3(global.epsilon, 0, 0))) -
                       ddist,
                   std::get<0>(sdfScene(hit_pos + vec3(0, global.epsilon, 0))) -
                       ddist,
                   std::get<0>(sdfScene(hit_pos + vec3(0, 0, global.epsilon))) -
                       ddist)),
          mat);
    }
  }
  return std::make_tuple(vec3(), vec3(), mat);
}

vec3 trace(const vec3 &origin, const vec3 &dir,
           std::shared_ptr<Material> medium = nullptr, std::size_t depth = 0) {
  if (depth >= global.bounces && frand() < 0.1f)
    return vec3(0.0f);
  auto [pos, normal, mat] = rayMarch(origin, dir);
  if (mat == nullptr)
    return global.sky;
  std::optional<vec3> new_dir = mat->sample(medium, dir, normal);
  vec3 ri(0.0f);
  if (new_dir.has_value()) {
    std::shared_ptr<Material> new_medium = nullptr;
    if (mat->type == Material::TRANSMISSIVE) {
      if (medium == mat) {
        new_dir = mat->sample(medium, dir, -normal);
      } else {
        new_medium = mat;
      }
    }
    ri = trace(pos + (2.0f * global.epsilon * new_dir.value()), new_dir.value(),
               new_medium, depth + 1);
  }
  return mat->evaluate(ri, new_dir.value_or(vec3(0.0)), dir, normal);
}

void render() {
  global.view = inverse(
      lookAt(global.camera_pos, global.camera_center, global.camera_up));
  float filmz = global.res.x / (2.00f * tan(global.fov / 2.0f));

  float *buffer =
      (float *)malloc(sizeof(float) * 3 * global.res.x * global.res.y);

  vec3 o = global.view * vec4(0.0, 0.0, 0.0, 1.0f);
  for (std::size_t i = 0; i < global.res.x * global.res.y; ++i) {
    std::size_t x = i % global.res.x;
    std::size_t y = i / global.res.x;
    for (std::size_t s = 0; s < global.samples; ++s) {
      vec3 dir(x - global.res.x / 2.0f + frand(), y - global.res.y / 2.0f,
               -filmz);
      vec3 color = clamp(trace(o, dir, nullptr), 0.0f, 1.0f);
      buffer[(i * 3) + 0] = color.r;
      buffer[(i * 3) + 1] = color.g;
      buffer[(i * 3) + 2] = color.b;
    }
  }

  write_file("test.png", buffer);

  if (buffer != nullptr)
    free(buffer);
}

int main(int argc, char *argv[]) {
  cxxopts::Options options("trm", "Tiny Ray Marcher");
  options.show_positional_help();
  options.add_options()(
      "o,output", "Output file path",
      cxxopts::value<std::string>()->default_value("{frame:010}.png"),
      "FILEDESC");
  options.add_options()("h,help", "show this help message");
  auto result = options.parse(argc, argv);
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  render();

  std::cout << "Output = " << result["output"].as<std::string>() << std::endl;

  return 0;
}
