#include <chrono>
#include <cstdio>
#include <fmt/format.h>
#include <regex>
#include <string>

class ProgressBar {
public:
  ProgressBar(const std::size_t &n, const std::string &desc = "");
  static std::size_t display_len(const std::string &str);
  static std::string format_sizeof(const float &number);
  static std::string format_interval(const std::size_t &s);
  template <typename T> static std::string format_number(const T &num) {
    std::string f = fmt::format("{:.3g}", num);
    if (f.find("+0") != std::string::npos)
      f.replace(f.find("+0"), 2, "+");
    if (f.find("-0") != std::string::npos)
      f.replace(f.find("-0"), 2, "-");
    std::string n = fmt::format("{}", num);
    return (f.size() < n.size()) ? f : n;
  }
  static std::string format_color(const char &spec, const char &msg);
  static std::string format_color(const char &spec, const std::string &msg);
  static std::string format_bar(const float &frac, const std::size_t len,
                                const std::string &chars = "[#>-]",
                                const std::string &colors = "*___*");
  void next(std::size_t i = 1);
  void display();
  void update(std::size_t i = 1);
  void finish();

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