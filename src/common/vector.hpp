#pragma once

namespace om {

/*
 * Vec2
 */

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

/*
 * Vec3
 */

template <typename T>
struct Vec3 {
  constexpr Vec3() = default;
  constexpr Vec3(T x, T y, T z) : x{x}, y{y}, z{z} {}
  constexpr explicit Vec3(T xyz) : x{xyz}, y{xyz}, z{xyz} {}

  T x;
  T y;
  T z;
};

template <typename T>
Vec3<T> operator+(const Vec3<T>& a, const Vec3<T>& b) {
  return Vec3<T>{a.x + b.x, a.y + b.y, a.z + b.z};
}

template <typename T>
Vec3<T> operator-(const Vec3<T>& a, const Vec3<T>& b) {
  return Vec3<T>{a.x - b.x, a.y - b.y, a.z - b.z};
}

template <typename T>
Vec3<T> operator*(const Vec3<T>& a, const Vec3<T>& b) {
  return Vec3<T>{a.x * b.x, a.y * b.y, a.z * b.z};
}

template <typename T>
Vec3<T> operator/(const Vec3<T>& a, const Vec3<T>& b) {
  return Vec3<T>{a.x / b.x, a.y / b.y, a.z / b.z};
}

using Vec3f = Vec3<float>;

}