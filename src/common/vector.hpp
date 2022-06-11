#pragma once

namespace om {

template <typename T>
struct Vec2 {
  constexpr Vec2() = default;
  constexpr Vec2(T x, T y) : x{x}, y{y} {}
  constexpr explicit Vec2(T xy) : x{xy}, y{xy} {}

  T x;
  T y;
};

template <typename T>
Vec2<T> operator+(const Vec2<T>& a, const Vec2<T>& b) {
  return Vec2<T>{a.x + b.x, a.y + b.y};
}

template <typename T>
Vec2<T> operator-(const Vec2<T>& a, const Vec2<T>& b) {
  return Vec2<T>{a.x - b.x, a.y - b.y};
}

template <typename T>
Vec2<T> operator*(const Vec2<T>& a, const Vec2<T>& b) {
  return Vec2<T>{a.x * b.x, a.y * b.y};
}

template <typename T>
Vec2<T> operator/(const Vec2<T>& a, const Vec2<T>& b) {
  return Vec2<T>{a.x / b.x, a.y / b.y};
}

using Vec2f = Vec2<float>;

}