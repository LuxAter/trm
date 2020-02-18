#ifndef TRM_SCENE_HPP_
#define TRM_SCENE_HPP_

#include <vector>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

struct Scene {
  Float maximum_distance = 1e3;
  Float epsilon_distance = 1e-3;
  Float fov = M_PI / 2.0f;
  uvec2 resolution = uvec2(500, 500);
  std::size_t maxium_depth = 16;
  std::size_t spp = 16;
  std::vector<std::shared_ptr<Sdf>> objects;
  std::vector<std::shared_ptr<Mat>> materials;
  Vec3 camera_pos, camera_center, camera_up;
};

#endif // TRM_SCENE_HPP_
