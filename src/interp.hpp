#include <glm/glm.hpp>

template <typename S, typename T>
T linear_interpolation(const T &a, const T &b, const S &t) {
  return a + t * (b - a);
}
template <typename S, typename T>
T finite_difference(const T& a, const T& b, const T& c, const T& d, const S& t) {

}

template <typename S, typename T> class Interpolation {
public:
  Interpolation(const std::function<T(const T &, const T &, const S &)>
                    &interp = linear_interpolation<S, T>)
      : values(), interp2(interp), interp4(nullptr) {}
  Interpolation(const std::function<T(const T &, const T &, const T &,
                                      const T &, const S &)> &interp)
      : values(), interp2(nullptr), interp4(interp) {}
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
  S begin() const { return values.front().first; }
  S end() const { return values.back().first; }
  T operator[](const S &t) {
    if (values.front().first > t) {
      return values.front().second;
    } else if (values.back().first < t) {
      return values.back().second;
    } else {
      typename std::vector<std::pair<S, T>>::iterator
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
