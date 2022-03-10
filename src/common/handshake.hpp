#pragma once

#include <atomic>
#include <optional>
#include <cassert>

namespace om {

template <typename T>
struct Handshake {
  std::atomic<bool> written{};
  std::atomic<bool> read{};
  bool awaiting_read{};
  T data;
};

//  by writer
template <typename T>
void publish(Handshake<T>* hs, T&& data) {
  assert(!hs->awaiting_read);
  hs->data = std::forward<T>(data);
  hs->awaiting_read = true;
  hs->written.store(true);
}

template <typename T>
bool acknowledged(Handshake<T>* hs) {
  assert(hs->awaiting_read);
  bool expect{true};
  if (hs->read.compare_exchange_strong(expect, false)) {
    hs->awaiting_read = false;
    return true;
  } else {
    return false;
  }
}

//  by consumer
template <typename T>
std::optional<T> read(Handshake<T>* hs) {
  bool expect{true};
  if (hs->written.compare_exchange_strong(expect, false)) {
    auto res = std::move(hs->data);
    hs->read.store(true);
    return res;
  } else {
    return std::nullopt;
  }
}

}