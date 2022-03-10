#pragma once

#include <array>
#include <atomic>
#include <memory>

namespace om {

namespace detail {
  struct CopyRange {
    template <typename T>
    static inline void apply(const T* begin, const T* end, T* dest) noexcept {
      std::copy(begin, end, dest);
    }
  };

  struct MoveRange {
    template <typename T>
    static inline void apply(const T* begin, const T* end, T* dest) noexcept {
      std::move(begin, end, dest);
    }
  };
}

/*
 * RingBuffer - A non-locking ring buffer that is thread-safe for 1 reader and 1 writer, with space
 * for N elements. This space is finite; if the buffer becomes full, no further writes are
 * possible.
 */

template <typename T, int N>
struct RingBufferStackStorage {
  std::array<T, N> buffer;
};

template <typename T, int N>
struct RingBufferHeapStorage {
  RingBufferHeapStorage() : buffer(new T[N]{}) {
    //
  }

  std::unique_ptr<T[]> buffer;
};

template <typename T, int N, typename Storage = RingBufferStackStorage<T, N>>
class RingBuffer {
private:
  static constexpr int capacity = N;

public:
  RingBuffer();

  template <typename U = T>
  void write(U&& element) noexcept;

  template <typename U = T>
  bool maybe_write(U&& element) noexcept;

  //  Copy elements in range [begin, end). Caller must ensure num_free() >= end - begin.
  void write_range_copy(const T* begin, const T* end) noexcept;
  //  Move elements in range [begin, end). Caller must ensure num_free() >= end - begin.
  void write_range_move(T* begin, T* end) noexcept;

  T read() noexcept;
  void clear() noexcept;

  //  Number of elements written and pending read.
  int size() const noexcept;
  //  Number of slots that can be written to.
  int num_free() const noexcept;
  bool full() const noexcept;

  int write_capacity() const noexcept {
    return N - 1;
  }

  T* begin();
  T* end();
  const T* begin() const;
  const T* end() const;

private:
  template <typename CopyOrMove, typename U>
  void write_range_impl(U* begin, U* end) noexcept;

private:
  Storage storage;

  std::atomic<int> rp{0};
  std::atomic<int> wp{0};
};

/*
 * Impl
 */

template <typename T, int N, typename Storage>
RingBuffer<T, N, Storage>::RingBuffer() {
  static_assert(N > 1, "Expected N > 1.");
}

template <typename T, int N, typename Storage>
template <typename U>
void RingBuffer<T, N, Storage>::write(U&& element) noexcept {
  auto w = wp.load();
  storage.buffer[w] = std::forward<U>(element);
  wp.store((w + 1) % capacity);
}

template <typename T, int N, typename Storage>
template <typename CopyOrMove, typename U>
void RingBuffer<T, N, Storage>::write_range_impl(U* begin, U* end) noexcept {
  auto size = end - begin;
  auto w = wp.load();
  auto free_space_ahead = capacity - w;
  auto forwards_size = size < free_space_ahead ? size : free_space_ahead;

  CopyOrMove::apply(begin, begin + forwards_size, this->begin() + w);

  if (forwards_size < size) {
    CopyOrMove::apply(begin + forwards_size, end, this->begin());
  }

  wp.store((w + size) % capacity);
}

template <typename T, int N, typename Storage>
void RingBuffer<T, N, Storage>::write_range_copy(const T* begin, const T* end) noexcept {
  write_range_impl<detail::CopyRange, const T>(begin, end);
}

template <typename T, int N, typename Storage>
void RingBuffer<T, N, Storage>::write_range_move(T* begin, T* end) noexcept {
  write_range_impl<detail::MoveRange, T>(begin, end);
}

template <typename T, int N, typename Storage>
template <typename U>
bool RingBuffer<T, N, Storage>::maybe_write(U&& element) noexcept {
  if (!full()) {
    write(std::forward<U>(element));
    return true;
  } else {
    return false;
  }
}

template <typename T, int N, typename Storage>
T RingBuffer<T, N, Storage>::read() noexcept {
  auto r = rp.load();
  T element = std::move(storage.buffer[r]);
  rp = (r + 1) % capacity;
  return element;
}

template <typename T, int N, typename Storage>
void RingBuffer<T, N, Storage>::clear() noexcept {
  int num_read = size();
  for (int i = 0; i < num_read; i++) {
    read();
  }
}

template <typename T, int N, typename Storage>
int RingBuffer<T, N, Storage>::size() const noexcept {
  int rp_value = rp % capacity;
  int wp_value = wp % capacity;

  if (rp_value <= wp_value) {
    return wp_value - rp_value;
  } else {
    return wp_value + (capacity - rp_value);
  }
}

template <typename T, int N, typename Storage>
int RingBuffer<T, N, Storage>::num_free() const noexcept {
  int rp_value = rp % capacity;
  int wp_value = wp % capacity;

  if (rp_value <= wp_value) {
    return capacity - (wp_value - rp_value) - 1;
  } else {
    return rp_value - wp_value - 1;
  }
}

template <typename T, int N, typename Storage>
bool RingBuffer<T, N, Storage>::full() const noexcept {
  return num_free() == 0;
}

template <typename T, int N, typename Storage>
T* RingBuffer<T, N, Storage>::begin() {
  return &storage.buffer[0];
}

template <typename T, int N, typename Storage>
T* RingBuffer<T, N, Storage>::end() {
  return &storage.buffer[0] + N;
}

template <typename T, int N, typename Storage>
const T* RingBuffer<T, N, Storage>::begin() const {
  return &storage.buffer[0];
}

template <typename T, int N, typename Storage>
const T* RingBuffer<T, N, Storage>::end() const {
  return &storage.buffer[0] + N;
}

}