#ifndef TRM_INTERP_HPP_
#define TRM_INTERP_HPP_

#include "type.hpp"
#include <algorithm>
#include <functional>
#include <map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace trm {
namespace interp {
  template <typename S, typename T>
  T linear_interpolation(const T &a, const T &b, const S &t) {
    return a + t * (b - a);
  }
  template <typename S, typename T>
  T hermite(const T &y0, const T &y1, const T &y2, const T &y3, const S &t) {
    S t2 = t * t;
    S t3 = t2 * t;
    T m0 = (y1 - y0) / 2.0f + (y2 - y1) / 2.0f;
    T m1 = (y2 - y1) / 2.0f + (y3 - y2) / 2.0f;
    S a0 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    S a1 = t3 - 2.0f * t2 + t;
    S a2 = t3 - t2;
    S a3 = -2 * t3 + 3.0f * t2;
    return (a0 * y1 + a1 * m0 + a2 * m1 + a3 * y2);
  }
  template <typename S> Quat slerp(const Quat &a, const Quat &b, const S &t) {
    return glm::slerp(a, b, t);
  }
  template <typename S>
  Quat squad(const Quat &a, const Quat &b, const Quat &c, const Quat &d,
             const S &t) {
    return glm::squad(a, d, b, c, t);
  }
} // namespace interp
template <typename S, typename T> class Interpolation {
public:
  Interpolation(const std::function<T(const T &, const T &, const S &)> &func =
                    trm::interp::linear_interpolation<S, T>)
      : values(), interp2(func), interp4(nullptr) {}
  Interpolation(const std::function<T(const T &, const T &, const T &,
                                      const T &, const S &)> &func)
      : values(), interp2(nullptr), interp4(func) {}
  Interpolation(const T &init,
                const std::function<T(const T &, const T &, const S &)> &func =
                    trm::interp::linear_interpolation<S, T>)
      : values({{0.0f, init}}), interp2(func), interp4(nullptr) {}
  Interpolation(const T &init,
                const std::function<T(const T &, const T &, const T &,
                                      const T &, const S &)> &func)
      : values({{0.0f, init}}), interp2(nullptr), interp4(func) {}
  void clear() { values.clear(); }
  void insert(const S &t, const T &v) {
    if (values.size() == 0) {
      values.push_back(std::make_pair(t, v));
    } else if (values.front().first > t) {
      values.insert(values.begin(), std::make_pair(t, v));
    } else if (values.back().first < t) {
      values.push_back(std::make_pair(t, v));
    } else {
      values.insert(std::find_if(values.begin(), values.end(),
                                 [t](const std::pair<S, T> &tv) {
                                   return tv.first > t;
                                 }) -
                        1,
                    std::make_pair(t, v));
    }
  }
  S begin(int offset = 0) const { return (values.begin() + offset)->first; }
  S end(int offset = 0) const { return (values.end() + offset - 1)->first; }
  T operator[](const S &t) const {
    if (values.size() == 1) {
      return values.front().second;
    } else if (values.front().first > t) {
      return values.front().second;
    } else if (values.back().first < t) {
      return values.back().second;
    } else {
      typename std::vector<std::pair<S, T>>::const_iterator
          next_it = std::find_if(
              values.begin(), values.end(),
              [t](const std::pair<S, T> &tv) { return tv.first > t; }),
          prev_it = next_it - 1;
      if (interp2 != nullptr) {
        return interp2(prev_it->second, next_it->second,
                       (t - prev_it->first) /
                           (next_it->first - prev_it->first));
      } else if (interp4 != nullptr) {
        return interp4((prev_it != values.begin()) ? (prev_it - 1)->second
                                                   : prev_it->second,
                       prev_it->second, next_it->second,
                       (next_it != values.end() - 1) ? (next_it + 1)->second
                                                     : next_it->second,
                       (t - prev_it->first) /
                           (next_it->first - prev_it->first));
      }
    }
    return T();
  }
  std::vector<std::pair<S, T>> values;
  std::function<T(const T &, const T &, const S &)> interp2;
  std::function<T(const T &, const T &, const T &, const T &, const S &)>
      interp4;
};
} // namespace trm

#endif // TRM_INTERP_HPP_
