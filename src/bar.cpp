#include "bar.hpp"

#include <chrono>
#include <cstdio>
#include <fmt/format.h>
#include <regex>
#include <string>

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

ProgressBar::ProgressBar(const std::size_t &n, const std::string &desc,
                         const bool &nice_display)
    : n(0), total(n), tp(std::chrono::system_clock::now()), desc(desc),
      nice_display(nice_display) {
  display();
}

std::size_t ProgressBar::display_len(const std::string &str) {
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
std::string ProgressBar::format_sizeof(const float &number) {
  float num = number;
  for (auto &&unit : {' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z'}) {
    if (std::abs(num) < 999.5) {
      if (std::abs(num) < 99.95) {
        if (std::abs(num) < 9.995) {
          return fmt::format("{:1.3f}", num) + unit;
        }
        return fmt::format("{:2.2f}", num) + unit;
      }
      return fmt::format("{:3.1f}", num) + unit;
    }
    num /= 1000;
  }
  return fmt::format("{0:3.1}Y", num);
}
std::string ProgressBar::format_interval(const std::size_t &s) {
  auto mins_secs = std::div(s, 60);
  auto hours_mins = std::div(mins_secs.quot, 60);
  if (hours_mins.quot != 0) {
    return fmt::format("{0:d}:{1:02d}:{2:02d}", hours_mins.quot, hours_mins.rem,
                       mins_secs.rem);
  } else {
    return fmt::format("{0:02d}:{1:02d}", hours_mins.rem, mins_secs.rem);
  }
}
std::string ProgressBar::format_color(const char &spec, const char &msg) {
  if (nice_display == false)
    return fmt::format("{:c}", msg);
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
std::string ProgressBar::format_color(const char &spec,
                                      const std::string &msg) {
  if (nice_display == false)
    return msg;
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
std::string ProgressBar::format_bar(const float &frac, const std::size_t len,
                                    const std::string &chars,
                                    const std::string &colors) {
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

void ProgressBar::next(std::size_t i) {
  n += i;
  std::chrono::system_clock::time_point now_tp =
      std::chrono::system_clock::now();
  elapsed += std::chrono::duration_cast<std::chrono::milliseconds>(now_tp - tp)
                 .count() /
             1e3;
  tp = now_tp;
}
void ProgressBar::display() {
  std::string elapsed_str = format_interval(elapsed);
  float rate = n / elapsed;
  float inv_rate = 1 / rate;
  std::string rate_noinv_str =
      (unit_scale ? format_sizeof(rate) : fmt::format("{:5.2f}", rate)) + unit +
      "/s";
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
      [percentage, this](const std::smatch &match) {
        std::string fmt_spec = "{:" + (match[2].matched ? match[2].str() : "") +
                               (match[4].matched ? "." + match[4].str() : "") +
                               "f}";
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
              fmt_color += std::string(5 - fmt_color.size(), fmt_color.back());
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
  if (nice_display == false) {
    std::fprintf(stdout, "%s\n", meter.c_str());
  } else {
    std::fprintf(stdout, "\033[2K\033[1G%s\033[0m", meter.c_str());
    std::fflush(stdout);
  }
}

void ProgressBar::update(std::size_t i) {
  next(i);
  display();
}
void ProgressBar::finish() {
  next(total - n);
  display();
  if (!nice_display)
    std::fprintf(stdout, "\n");
}
