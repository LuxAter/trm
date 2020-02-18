#define GLM_FORCE_SWIZZLE
#define GLM_LEFT_HANDED

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
