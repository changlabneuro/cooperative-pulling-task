#pragma once

namespace om {

template <typename T, typename U>
inline T lerp(U frac, const T& a, const T& b) {
  return (U(1) - frac) * a + frac * b;
}

}