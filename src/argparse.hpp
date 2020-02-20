#ifndef ARGPARSE_HPP_RH4N18TX
#define ARGPARSE_HPP_RH4N18TX

#include <cstdio>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <iostream>

namespace trm {
namespace argparse {
  bool parse_opt(const std::string &arg, int *value) {
    return std::sscanf(arg.c_str(), "%d", value) == 1;
  }
  bool parse_opt(const std::string &arg, std::size_t *value) {
    return std::sscanf(arg.c_str(), "%lu", value) == 1;
  }
  bool parse_opt(const std::string &arg, float *value) {
    return std::sscanf(arg.c_str(), "%f", value) == 1;
  }
  bool parse_opt(const std::string &arg, double *value) {
    return std::sscanf(arg.c_str(), "%lf", value) == 1;
  }
  bool parse_opt(const std::string &arg, glm::uvec2 *value) {
    return std::sscanf(arg.c_str(), "%u,%u", &value->x, &value->y) == 2;
  }
  bool parse_opt(const std::string &arg, glm::vec2 *value) {
    return std::sscanf(arg.c_str(), "%f,%f", &value->x, &value->y) == 2;
  }
  bool parse_opt(const std::string &arg, glm::uvec3 *value) {
    return std::sscanf(arg.c_str(), "%u,%u,%u", &value->x, &value->y,
                       &value->z) == 3;
  }
  bool parse_opt(const std::string &arg, glm::vec3 *value) {
    return std::sscanf(arg.c_str(), "%f,%f,%f", &value->x, &value->y,
                       &value->z) == 3;
  }
  bool parse_opt(const std::string &arg, std::string *value) {
    char buf[255];
    int result = std::sscanf(arg.c_str(), "%s", buf);
    if (result != 1)
      return false;
    *value = std::string(buf);
    return true;
  }

  class Parser {
  public:
    class Argument {
    public:
      enum Type { BOOL, INT, UINT, FLOAT, UVEC2, VEC2, UVEC3, VEC3, STRING };

      Argument(const std::string expr, bool *ptr, std::string help)
          : type(BOOL), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}
      Argument(const std::string expr, int *ptr, std::string help)
          : type(INT), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}
      Argument(const std::string expr, std::size_t *ptr, std::string help)
          : type(INT), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}
      Argument(const std::string expr, float *ptr, std::string help)
          : type(FLOAT), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}
      Argument(const std::string expr, glm::uvec2 *ptr, std::string help)
          : type(UVEC2), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}
      Argument(const std::string expr, glm::vec2 *ptr, std::string help)
          : type(VEC2), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}
      Argument(const std::string expr, glm::uvec3 *ptr, std::string help)
          : type(UVEC3), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}
      Argument(const std::string expr, glm::vec3 *ptr, std::string help)
          : type(VEC3), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}
      Argument(const std::string expr, std::string *ptr, std::string help)
          : type(STRING), ptr(reinterpret_cast<void *>(ptr)), expr(expr + ','),
            help(help) {}

      inline void print_help(const std::size_t width) const {
        std::printf("  %*s  %s\n", static_cast<int>(width),
                    expr.substr(0, expr.size() - 1).c_str(), help.c_str());
      }

      inline int match(const std::string &arg, const std::string &peek) {
        std::size_t prev = 0, pos = 0;
        while ((pos = expr.find(",", prev)) != std::string::npos) {
          std::string matcher = expr.substr(prev, pos - prev);
          prev = pos + 1;
          if (arg.compare(0, matcher.size(), matcher) == 0) {
            if (type == BOOL) {
              *reinterpret_cast<bool *>(ptr) = true;
              return matcher.size();
            } else if (arg.size() > matcher.size() &&
                       arg[matcher.size()] == '=') {
              switch (type) {
              case INT:
                if (!parse_opt(arg.substr(matcher.size() + 1),
                               reinterpret_cast<int *>(ptr)))
                  return -1;
                break;
              case UINT:
                if (!parse_opt(arg.substr(matcher.size() + 1),
                               reinterpret_cast<std::size_t *>(ptr)))
                  return -1;
                break;
              case FLOAT:
                if (!parse_opt(arg.substr(matcher.size() + 1),
                               reinterpret_cast<float *>(ptr)))
                  return -1;
                break;
              case UVEC2:
                if (!parse_opt(arg.substr(matcher.size() + 1),
                               reinterpret_cast<glm::uvec2 *>(ptr)))
                  return -1;
                break;
              case VEC2:
                if (!parse_opt(arg.substr(matcher.size() + 1),
                               reinterpret_cast<glm::vec2 *>(ptr)))
                  return -1;
                break;
              case UVEC3:
                if (!parse_opt(arg.substr(matcher.size() + 1),
                               reinterpret_cast<glm::uvec3 *>(ptr)))
                  return -1;
                break;
              case VEC3:
                if (!parse_opt(arg.substr(matcher.size() + 1),
                               reinterpret_cast<glm::vec3 *>(ptr)))
                  return -1;
                break;
              case STRING:
                if (!parse_opt(arg.substr(matcher.size() + 1),
                               reinterpret_cast<std::string *>(ptr)))
                  return -1;
                break;
              default:
                return -1;
              }
              return arg.size();
            } else {
              switch (type) {
              case INT:
                if (!parse_opt(peek, reinterpret_cast<int *>(ptr)))
                  return -1;
                break;
              case UINT:
                if (!parse_opt(peek, reinterpret_cast<std::size_t *>(ptr)))
                  return -1;
                break;
              case FLOAT:
                if (!parse_opt(peek, reinterpret_cast<float *>(ptr)))
                  return -1;
                break;
              case UVEC2:
                if (!parse_opt(peek, reinterpret_cast<glm::uvec2 *>(ptr)))
                  return -1;
                break;
              case VEC2:
                if (!parse_opt(peek, reinterpret_cast<glm::vec2 *>(ptr)))
                  return -1;
                break;
              case UVEC3:
                if (!parse_opt(peek, reinterpret_cast<glm::uvec3 *>(ptr)))
                  return -1;
                break;
              case VEC3:
                if (!parse_opt(peek, reinterpret_cast<glm::vec3 *>(ptr)))
                  return -1;
                break;
              case STRING:
                if (!parse_opt(peek, reinterpret_cast<std::string *>(ptr)))
                  return -1;
                break;
              default:
                return -1;
              }
              return arg.size() + peek.size();
            }
          }
        }
        return 0;
      }
      Type type;
      void *ptr;
      std::string expr, help;
    };

    Parser(const std::string desc) : desc(desc) {}
    template <typename T>
    Parser &add(std::string expr, T *ptr, const std::string &&help = "") {
      args.push_back(Argument(expr, ptr, help));
      return *this;
    }
    template <typename T>
    Parser &add(std::string expr, const std::string &help, T *ptr) {
      args.push_back(Argument(expr, ptr, help));
      return *this;
    }

    bool parse(const int argc, char *argv[]) {
      exec = argv[0];
      std::vector<std::string> cargs;
      for (int i = 1; i < argc; ++i) {
        cargs.push_back(argv[i]);
      }

      while (cargs.size() != 0) {
        std::string current = cargs[0];
        for (auto &arg : args) {
          int chars = arg.match(cargs[0], cargs.size() != 1 ? cargs[1] : "");
          if (chars < 0) {
            std::fprintf(stderr, "Command line argument error\n");
            return false;
          } else if (chars == 2 && cargs[0].size() > 2 && cargs[0][1] != '-') {
            cargs[0] = '-' + cargs[0].substr(chars);
            break;
          } else if (chars >= static_cast<int>(cargs[0].size())) {
            chars -= cargs[0].size();
            cargs.erase(cargs.begin());
            if (chars == static_cast<int>(cargs[0].size())) {
              cargs.erase(cargs.begin());
            }
            break;
          }
        }
        if (cargs.size() != 0 && cargs[0] == current) {
          std::fprintf(stderr, "Unrecognized command line argument \"%s\"\n",
                       current.c_str());
          return false;
        }
      }
      return true;
    }

    void help() {
      std::printf("Usage: %s [OPTIONS]\n", exec.c_str());
      std::printf("%s\n\n", desc.c_str());

      std::size_t max_width = 0;
      for (auto &arg : args) {
        max_width = std::max(max_width, arg.expr.size());
      }
      for (auto &arg : args) {
        arg.print_help(max_width);
      }
    }

    std::string exec, desc;
    std::vector<Argument> args;
  };
} /* namespace argparse */
} // namespace trm

#endif /* end of include guard: ARGPARSE_HPP_RH4N18TX */
