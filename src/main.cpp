#include <bits/c++config.h>
#include <cstdio>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <regex>
#include <string>
#include <tuple>

#define GLM_FORCE_SWIZZLE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define GLM_LEFT_HANDED

#include <cxxopts.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image_write.h"

using namespace glm;
namespace std {
template <class BidirIt, class Traits, class CharT, class UnaryFunction>
std::basic_string<CharT>
regex_replace(BidirIt first, BidirIt last,
              const std::basic_regex<CharT, Traits> &re, UnaryFunction f) {
  std::basic_string<CharT> s;

  typename std::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
  auto endOfLastMatch = first;

  auto callback = [&](const std::match_results<BidirIt> &match) {
    auto positionOfThisMatch = match.position(0);
    auto diff = positionOfThisMatch - positionOfLastMatch;

    auto startOfThisMatch = endOfLastMatch;
    std::advance(startOfThisMatch, diff);

    s.append(endOfLastMatch, startOfThisMatch);
    s.append(f(match));

    auto lengthOfMatch = match.length(0);

    positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

    endOfLastMatch = startOfThisMatch;
    std::advance(endOfLastMatch, lengthOfMatch);
  };

  std::regex_iterator<BidirIt> begin(first, last, re), end;
  std::for_each(begin, end, callback);

  s.append(endOfLastMatch, last);

  return s;
}

template <class Traits, class CharT, class UnaryFunction>
std::string regex_replace(const std::string &s,
                          const std::basic_regex<CharT, Traits> &re,
                          UnaryFunction f) {
  return regex_replace(s.cbegin(), s.cend(), re, f);
}
} // namespace std

#ifdef DOUBLE_PERC
typedef glm::dvec3 Vec3;
typedef glm::dvec4 Vec4;
typedef glm::dmat4 Mat4;
typedef double Float;
#else
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;
typedef glm::mat4 Mat4;
typedef float Float;
#endif
struct Sdf;
struct Mat;

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
static uvec2 resolution = uvec2(500, 500);
static std::size_t maximum_depth = 10;
static std::size_t spp = 32;
static std::vector<std::shared_ptr<Sdf>> objects;
static Camera camera;

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<Float> dist(0.0f, 1.0f);
std::uniform_real_distribution<Float> dist2(-1.0f, 1.0f);
inline Float frand() { return dist(gen); }
inline Float frand2() { return dist2(gen); }

class ProgressBar {
public:
  ProgressBar(const std::size_t &n, const std::string &desc = "")
      : n(0), total(n), tp(std::chrono::system_clock::now()), desc(desc) {
    display();
  }

  static std::size_t display_len(const std::string &str) {
    std::size_t len = 0;
    bool skip = false;
    for (std::size_t i = 0; i < str.size(); ++i) {
      if (str[i] == '\033')
        skip = true;
      else if (skip && str[i] == 'm')
        skip = false;
      else
        len++;
    }
    return len;
  }
  static std::string format_sizeof(const float &number) {
    float num = number;
    for (auto &&unit : {' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z'}) {
      if (std::abs(num) < 999.5) {
        if (std::abs(num) < 99.95) {
          if (std::abs(num) < 9.995) {

            return fmt::format("{:1.2f}", num) + unit;
          }
          return fmt::format("{:2.1f}", num) + unit;
        }
        return fmt::format("{:3.1f}", num) + unit;
      }
      num /= 1000;
    }
    return fmt::format("{0:3.1}Y", num);
  }
  static std::string format_interval(const std::size_t &s) {
    auto mins_secs = std::div(s, 60);
    auto hours_mins = std::div(mins_secs.quot, 60);
    if (hours_mins.quot != 0) {
      return fmt::format("{0:d}:{1:02d}:{2:02d}", hours_mins.quot,
                         hours_mins.rem, mins_secs.rem);
    } else {
      return fmt::format("{0:02d}:{1:02d}", hours_mins.rem, mins_secs.rem);
    }
  }
  template <typename T> static std::string format_number(const T &num) {
    std::string f = fmt::format("{:.3g}", num);
    if (f.find("+0") != std::string::npos)
      f.replace(f.find("+0"), 2, "+");
    if (f.find("-0") != std::string::npos)
      f.replace(f.find("-0"), 2, "-");
    std::string n = fmt::format("{}", num);
    return (f.size() < n.size()) ? f : n;
  }
  static std::string format_color(const char &spec, const char &msg) {
    switch (spec) {
    case 'k':
      return fmt::format("\033[30m{}\033[0m", msg);
    case 'r':
      return fmt::format("\033[31m{}\033[0m", msg);
    case 'g':
      return fmt::format("\033[32m{}\033[0m", msg);
    case 'y':
      return fmt::format("\033[33m{}\033[0m", msg);
    case 'b':
      return fmt::format("\033[34m{}\033[0m", msg);
    case 'm':
      return fmt::format("\033[35m{}\033[0m", msg);
    case 'c':
      return fmt::format("\033[36m{}\033[0m", msg);
    case 'w':
      return fmt::format("\033[37m{}\033[0m", msg);
    case 'K':
      return fmt::format("\033[1;30m{}\033[0m", msg);
    case 'R':
      return fmt::format("\033[1;31m{}\033[0m", msg);
    case 'G':
      return fmt::format("\033[1;32m{}\033[0m", msg);
    case 'Y':
      return fmt::format("\033[1;33m{}\033[0m", msg);
    case 'B':
      return fmt::format("\033[1;34m{}\033[0m", msg);
    case 'M':
      return fmt::format("\033[1;35m{}\033[0m", msg);
    case 'C':
      return fmt::format("\033[1;36m{}\033[0m", msg);
    case 'W':
      return fmt::format("\033[1;37m{}\033[0m", msg);
    case '*':
      return fmt::format("\033[1m{}\033[0m", msg);
    default:
      return fmt::format("{:c}", msg);
    }
  }
  static std::string format_color(const char &spec, const std::string &msg) {
    switch (spec) {
    case 'k':
      return fmt::format("\033[30m{}\033[0m", msg);
    case 'r':
      return fmt::format("\033[31m{}\033[0m", msg);
    case 'g':
      return fmt::format("\033[32m{}\033[0m", msg);
    case 'y':
      return fmt::format("\033[33m{}\033[0m", msg);
    case 'b':
      return fmt::format("\033[34m{}\033[0m", msg);
    case 'm':
      return fmt::format("\033[35m{}\033[0m", msg);
    case 'c':
      return fmt::format("\033[36m{}\033[0m", msg);
    case 'w':
      return fmt::format("\033[37m{}\033[0m", msg);
    case 'K':
      return fmt::format("\033[1;30m{}\033[0m", msg);
    case 'R':
      return fmt::format("\033[1;31m{}\033[0m", msg);
    case 'G':
      return fmt::format("\033[1;32m{}\033[0m", msg);
    case 'Y':
      return fmt::format("\033[1;33m{}\033[0m", msg);
    case 'B':
      return fmt::format("\033[1;34m{}\033[0m", msg);
    case 'M':
      return fmt::format("\033[1;35m{}\033[0m", msg);
    case 'C':
      return fmt::format("\033[1;36m{}\033[0m", msg);
    case 'W':
      return fmt::format("\033[1;37m{}\033[0m", msg);
    case '*':
      return fmt::format("\033[1m{}\033[0m", msg);
    default:
      return msg;
    }
  }
  static std::string format_bar(const float &frac, const std::size_t len,
                                const std::string &chars = "[#>-]",
                                const std::string &colors = "*___*") {
    std::size_t bar_length = std::floor(frac * len);
    std::string bar(bar_length, chars[1]);
    if (bar_length < len) {
      return format_color(colors[0], chars[0]) + format_color(colors[1], bar) +
             format_color(colors[2], chars[2]) +
             format_color(colors[3],
                          std::string(len - bar_length - 1, chars[3])) +
             format_color(colors[4], chars[4]);
    } else {
      return format_color(colors[0], chars[0]) + format_color(colors[1], bar) +
             format_color(colors[4], chars[4]);
    }
  }

  inline void next(std::size_t i = 1) {
    n += i;
    std::chrono::system_clock::time_point now_tp =
        std::chrono::system_clock::now();
    elapsed +=
        std::chrono::duration_cast<std::chrono::milliseconds>(now_tp - tp)
            .count() /
        1e3;
    tp = now_tp;
  }
  inline void display() {
    std::string elapsed_str = format_interval(elapsed);
    float rate = n / elapsed;
    float inv_rate = 1 / rate;
    std::string rate_noinv_str =
        (unit_scale ? format_sizeof(rate) : fmt::format("{:5.2f}", rate)) +
        unit + "/s";
    std::string rate_inv_str = (unit_scale ? format_sizeof(inv_rate)
                                           : fmt::format("{:5.2f}", inv_rate)) +
                               "s/" + unit;
    std::string rate_str = inv_rate > 1 ? rate_inv_str : rate_noinv_str;
    std::string n_str = unit_scale ? format_sizeof(n) : fmt::format("{}", n);
    std::string total_str =
        unit_scale ? format_sizeof(total) : fmt::format("{}", total);
    float remaining = (total - n) / rate;
    std::string remaining_str = format_interval(remaining);

    float frac = static_cast<float>(n) / static_cast<float>(total);
    float percentage = frac * 100.0;

    std::string meter = format_spec;
    std::smatch results;
    try {
      std::regex tmp("\\{percentage\\}");
    } catch (std::regex_error &e) {
      std::fprintf(stderr, "Regex: %s\n", e.what());
    }
    meter = std::regex_replace(
        meter,
        std::regex("\\{percentage(:([0-9]+)?(\\.([0-9]+))?)?(;(["
                   "krgybmcwKRGYBMCW\\*_]+))?\\}"),
        [percentage](const std::smatch &match) {
          std::string fmt_spec =
              "{:" + (match[2].matched ? match[2].str() : "") +
              (match[4].matched ? "." + match[4].str() : "") + "f}";
          return format_color(match[5].matched ? match[5].str()[0] : '_',
                              fmt::format(fmt_spec, percentage));
        });
    for (std::string &&key :
         {"n", "total", "elapsed", "remaining", "rate", "bar", "desc"}) {
      meter = std::regex_replace(
          meter,
          std::regex("\\{" + key +
                     "(:(<|^|>)?([0-9]+)?)?(;([krgybmcKRGYBMCW\\*_]+))?\\}"),
          [key, n_str, total_str, elapsed_str, remaining_str, rate_str, frac,
           this](const std::smatch &match) {
            std::string val = "";
            char align = match[2].matched ? match[2].str()[0] : '>';
            int fmt_width = match[3].matched ? std::stoi(match[3]) : -1;
            std::string fmt_color = match[5].matched ? match[5].str() : "_____";
            if (key == "n") {
              val = n_str;
            } else if (key == "total") {
              val = total_str;
            } else if (key == "elapsed") {
              val = elapsed_str;
            } else if (key == "remaining") {
              val = remaining_str;
            } else if (key == "rate") {
              val = rate_str;
            } else if (key == "bar") {
              if (fmt_color.size() < 5) {
                fmt_color +=
                    std::string(5 - fmt_color.size(), fmt_color.back());
              }
              val = format_bar(frac, fmt_width != -1 ? fmt_width : 20,
                               this->bar_chars, fmt_color);
            } else if (key == "desc") {
              val = this->desc;
            }
            if (fmt_width != -1 &&
                static_cast<int>(display_len(val)) < fmt_width) {
              std::size_t str_disp_len = display_len(val);
              if (align == '<') {
                val = val + std::string(fmt_width - str_disp_len, ' ');
              } else if (align == '^') {
                val = std::string((fmt_width - str_disp_len) / 2, ' ') + val +
                      std::string((fmt_width - str_disp_len) / 2, ' ');

              } else {
                val = std::string(fmt_width - str_disp_len, ' ') + val;
              }
            }
            return format_color(fmt_color[0], val);
          });
    }
    std::fprintf(stdout, "\033[2K\033[1G%s\033[0m", meter.c_str());
    std::fflush(stdout);
  }

  inline void update(std::size_t i = 1) {
    next(i);
    display();
  }
  inline void finish() {
    next(total - n);
    display();
  }

  std::size_t n, total;
  float elapsed = 0.0f;
  std::chrono::system_clock::time_point tp;

  bool unit_scale = false;
  std::string desc;
  std::string unit = "it";
  std::string bar_chars = "[=>-]";
  std::string format_spec = "{desc;W} {percentage:3.0}% {bar:40;WCCcW} "
                            "{n}/{total} [{elapsed}<{remaining}, {rate;B}]";
};

// File writing interface
void write_file(const std::string &file_desc, const uvec2 &res,
                const uint8_t *raw) {
  stbi_flip_vertically_on_write(true);
  if (file_desc.ends_with(".png")) {
    stbi_write_png(file_desc.c_str(), res.x, res.y, 3, raw,
                   res.x * 3 * sizeof(uint8_t));
  } else if (file_desc.ends_with(".bmp")) {
    stbi_write_bmp(file_desc.c_str(), res.x, res.y, 3, raw);
  } else if (file_desc.ends_with(".tga")) {
    stbi_write_tga(file_desc.c_str(), res.x, res.y, 3, raw);
  } else if (file_desc.ends_with(".jpg")) {
    stbi_write_jpg(file_desc.c_str(), res.x, res.y, 3, raw, 75);
  }
}

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
    return abs(this->dist(Vec3(inv * Vec4(p, 1.0))));
  }
  inline Vec3 normal(const Vec3 &p,
                     const Float &ep = 10 *
                                       std::numeric_limits<Float>::epsilon()) {
    Vec3 op = inv * Vec4(p, 1.0f);
    return normalize(Vec4(this->dist(Vec3(op.x + ep, op.y, op.z)) -
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

std::ostream &operator<<(std::ostream &out, const Vec3 &v) {
  return out << fmt::format("[{}, {}, {}]", v.x, v.y, v.z);
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
    Float obj_dist = (*obj)(p);
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
  auto [t, obj] = rayMarch(r);
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

void render() {
  ProgressBar bar(resolution.y * resolution.x);
  bar.unit_scale = true;
  bar.unit = "px";
  uint8_t *buffer =
      (uint8_t *)malloc(sizeof(uint8_t) * 3 * resolution.x * resolution.y);

  Mat4 view = inverse(lookAtLH(camera.pos, camera.center, camera.up));
  Float filmz = resolution.x / (2.0f * tan(fov / 2.0f));
  Vec3 origin = view * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
#pragma omp parallel for shared(buffer, bar)
  for (std::size_t i = 0; i < resolution.x * resolution.y; ++i) {
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
  write_file("test.png", resolution, buffer);
  if (buffer != nullptr)
    free(buffer);
  bar.finish();
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

  camera.pos = Vec3(0.0f, 0.0f, 0.01f);
  camera.up = Vec3(0.0f, 1.0f, 0.0f);
  camera.center = Vec3(0.0f, 0.0f, 5.0f);

  objects.push_back(sdfPlane({0.0, 1.0, 0.0, -2.0}, matDiff({1.0, 1.0, 1.0})));
  objects.push_back(
      sdfPlane({0.0, -1.0, 0.0, -2.0}, matEmis(10.0, {1.0, 1.0, 1.0})));
  objects.push_back(sdfPlane({-1.0, 0.0, 0.0, -2.0}, matDiff({1.0, 0.2, 0.2})));
  objects.push_back(sdfPlane({1.0, 0.0, 0.0, -2.0}, matDiff({0.2, 1.0, 0.2})));
  objects.push_back(sdfPlane({0.0, 0.0, -1.0, -4.0}, matDiff({0.2, 0.2, 1.0})));
  objects.push_back(sdfPlane({0.0, 0.0, 1.0, 0.0}, matDiff({1.0, 1.0, 1.0})));

  objects.push_back(
      sdfSphere(1.0, matDiff({1.0, 1.0, 1.0}))->translate(1.0, -1.0, 3.0));
  objects.push_back(
      sdfSphere(0.5, matSpec({1.0, 1.0, 1.0}))->translate(-0.5, -1.5, 2.0));
  objects.push_back(sdfSphere(0.5, matRefr(1.5, {1.0, 1.0, 1.0}))
                        ->translate(-1.0, -1.5, 3.0));
  // objects.push_back(
  //     sdfSphere(0.5, matRefr(1.5, {1.0, 1.0, 1.0}))->translate(0.0,
  //     0.0, 2.0));

  render();

  std::cout << "Output = " << result["output"].as<std::string>() << std::endl;

  return 0;
}
