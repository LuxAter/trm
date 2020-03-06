#ifndef ARGPARSE_HPP_RH4N18TX
#define ARGPARSE_HPP_RH4N18TX

#include <cstdio>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <iostream>

namespace trm {
namespace argparse {
  bool opt(const std::string &arg, int *value);
  bool opt(const std::string &arg, std::size_t *value);
  bool opt(const std::string &arg, float *value);
  bool opt(const std::string &arg, double *value);
  bool opt(const std::string &arg, glm::vec2 *value);
  bool opt(const std::string &arg, glm::vec3 *value);
  bool opt(const std::string &arg, glm::uvec2 *value);
  bool opt(const std::string &arg, glm::uvec3 *value);
  bool opt(const std::string &arg, std::string *value);

  class Parser {
  public:
    class Argument {
    public:
      enum Type { BOOL, INT, UINT, FLOAT, UVEC2, VEC2, UVEC3, VEC3, STRING };
      Argument(const std::string expr, bool *ptr, std::string help);
      Argument(const std::string expr, int *ptr, std::string help);
      Argument(const std::string expr, std::size_t *ptr, std::string help);
      Argument(const std::string expr, float *ptr, std::string help);
      Argument(const std::string expr, glm::uvec2 *ptr, std::string help);
      Argument(const std::string expr, glm::uvec3 *ptr, std::string help);
      Argument(const std::string expr, glm::vec2 *ptr, std::string help);
      Argument(const std::string expr, glm::vec3 *ptr, std::string help);
      Argument(const std::string expr, std::string *ptr, std::string help);

      inline void print_help(const std::size_t width) const {
        std::printf("  %*s  %s\n", static_cast<int>(width),
                    expr.back() == ',' ? expr.substr(0, expr.size() - 1).c_str()
                                       : expr.c_str(),
                    help.c_str());
      }

      int parse_boolean(const std::string &arg);
      int parse_opt(const std::string &str);

      inline int match(const std::string &arg, const std::string &peek) {
        std::size_t prev = 0, pos = 0;
        if (expr[0] != '-') {
          if (type == BOOL)
            return -1;
          else {
            return parse_opt(arg);
          }
          return 0;
        }
        while ((pos = expr.find(",", prev)) != std::string::npos) {
          std::string matcher = expr.substr(prev, pos - prev);
          prev = pos + 1;
          if (arg.compare(0, matcher.size(), matcher) == 0) {
            if (type == BOOL) {
              return parse_boolean(matcher);
            } else if (arg.size() > matcher.size() &&
                       arg[matcher.size()] == '=') {
              return parse_opt(arg.substr(matcher.size() + 1)) == -1
                         ? -1
                         : arg.size();
            } else {
              return arg.size() + parse_opt(peek);
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
        bool matched = false;
        for (auto &arg : args) {
          if (arg.expr[0] != '-')
            continue;
          int chars = arg.match(cargs[0], cargs.size() != 1 ? cargs[1] : "");
          if (chars < 0) {
            std::fprintf(stderr, "Command line argument error\n");
            return false;
          } else if (chars == 2 && cargs[0].size() > 2 && cargs[0][1] != '-') {
            cargs[0] = '-' + cargs[0].substr(chars);
            matched = true;
            break;
          } else if (chars >= static_cast<int>(cargs[0].size())) {
            matched = true;
            chars -= cargs[0].size();
            cargs.erase(cargs.begin());
            if (chars == static_cast<int>(cargs[0].size())) {
              cargs.erase(cargs.begin());
            }
            break;
          }
        }
        if(matched == true) continue;
        for (auto &arg : args) {
          if(cargs.size() == 0) break;
          if (arg.expr[0] == '-')
            continue;
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
      std::printf("Usage: %s [OPTIONS]", exec.c_str());
      for(auto& arg : args) {
        if(arg.expr[0] == '-') continue;
        std::printf(" [%s]", arg.expr.substr(0, arg.expr.size() - 1).c_str());
      }
      std::printf("\n%s\n\n", desc.c_str());

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
