#ifndef TRM_RAND_HPP_
#define TRM_RAND_HPP_

#include "type.hpp"
#include <random>

namespace trm {
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<int>
    idist(0, std::numeric_limits<int>::max());
static std::uniform_real_distribution<Float> dist(0.0f, 1.0f);
static std::uniform_real_distribution<Float> dist2(-1.0f, 1.0f);
inline int irand() { return idist(gen); }
inline Float frand() { return dist(gen); }
inline Float frand2() { return dist2(gen); }
inline Float frand(const Float &min, const Float &max) {
  return dist(gen) * (max - min) + min;
}
} // namespace trm

#endif // TRM_RAND_HPP_
