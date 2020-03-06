#include "argparse.hpp"

#include <cstdio>
#include <glm/glm.hpp>
#include <string>

bool trm::argparse::opt(const std::string &arg, int *value) {
  return std::sscanf(arg.c_str(), "%d", value) == 1;
}
bool trm::argparse::opt(const std::string &arg, std::size_t *value) {
  return std::sscanf(arg.c_str(), "%lu", value) == 1;
}
bool trm::argparse::opt(const std::string &arg, float *value) {
  return std::sscanf(arg.c_str(), "%f", value) == 1;
}
bool trm::argparse::opt(const std::string &arg, double *value) {
  return std::sscanf(arg.c_str(), "%lf", value) == 1;
}
bool trm::argparse::opt(const std::string &arg, glm::uvec2 *value) {
  return std::sscanf(arg.c_str(), "%ux%u", &value->x, &value->y) == 2;
}
bool trm::argparse::opt(const std::string &arg, glm::vec2 *value) {
  return std::sscanf(arg.c_str(), "%f,%f", &value->x, &value->y) == 2;
}
bool trm::argparse::opt(const std::string &arg, glm::uvec3 *value) {
  return std::sscanf(arg.c_str(), "%u,%u,%u", &value->x, &value->y,
                     &value->z) == 3;
}
bool trm::argparse::opt(const std::string &arg, glm::vec3 *value) {
  return std::sscanf(arg.c_str(), "%f,%f,%f", &value->x, &value->y,
                     &value->z) == 3;
}
bool trm::argparse::opt(const std::string &arg, std::string *value) {
  char buf[255];
  int result = std::sscanf(arg.c_str(), "%s", buf);
  if (result != 1)
    return false;
  *value = std::string(buf);
  return true;
}

trm::argparse::Parser::Argument::Argument(const std::string expr, bool *ptr,
                                          std::string help)
    : type(BOOL), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}
trm::argparse::Parser::Argument::Argument(const std::string expr, int *ptr,
                                          std::string help)
    : type(INT), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}
trm::argparse::Parser::Argument::Argument(const std::string expr,
                                          std::size_t *ptr, std::string help)
    : type(UINT), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}
trm::argparse::Parser::Argument::Argument(const std::string expr, float *ptr,
                                          std::string help)
    : type(FLOAT), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}
trm::argparse::Parser::Argument::Argument(const std::string expr,
                                          glm::uvec2 *ptr, std::string help)
    : type(UVEC2), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}
trm::argparse::Parser::Argument::Argument(const std::string expr,
                                          glm::uvec3 *ptr, std::string help)
    : type(UVEC3), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}
trm::argparse::Parser::Argument::Argument(const std::string expr,
                                          glm::vec2 *ptr, std::string help)
    : type(VEC2), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}
trm::argparse::Parser::Argument::Argument(const std::string expr,
                                          glm::vec3 *ptr, std::string help)
    : type(VEC3), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}
trm::argparse::Parser::Argument::Argument(const std::string expr,
                                          std::string *ptr, std::string help)
    : type(STRING), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ","),
      help(help) {}

int trm::argparse::Parser::Argument::parse_boolean(const std::string &arg) {
  *reinterpret_cast<bool *>(ptr) = true;
  return arg.size();
}
int trm::argparse::Parser::Argument::parse_opt(const std::string &str) {
  switch (type) {
  case INT:
    if (!opt(str, reinterpret_cast<int *>(ptr)))
      return -1;
    break;
  case UINT:
    if (!opt(str, reinterpret_cast<std::size_t *>(ptr)))
      return -1;
    break;
  case FLOAT:
    if (!opt(str, reinterpret_cast<float *>(ptr)))
      return -1;
    break;
  case UVEC2:
    if (!opt(str, reinterpret_cast<glm::uvec2 *>(ptr)))
      return -1;
    break;
  case UVEC3:
    if (!opt(str, reinterpret_cast<glm::uvec3 *>(ptr)))
      return -1;
    break;
  case VEC2:
    if (!opt(str, reinterpret_cast<glm::vec2 *>(ptr)))
      return -1;
    break;
  case VEC3:
    if (!opt(str, reinterpret_cast<glm::vec3 *>(ptr)))
      return -1;
    break;
  case STRING:
    if (!opt(str, reinterpret_cast<std::string *>(ptr)))
      return -1;
    break;
  default:
    return -1;
  }
  return str.size();
}
