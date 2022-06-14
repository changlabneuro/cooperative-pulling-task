#pragma once

namespace om {

template <typename T>
T clamp(const T& v, const T& lo, const T& hi) {
  return v < lo ? lo : v > hi ? hi : v;
}

template <typename T, typename U>
inline T lerp(U frac, const T& a, const T& b) {
  return (U(1) - frac) * a + frac * b;
}

}