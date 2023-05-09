#pragma once

#include <vector>

namespace om {

template <typename T>
struct SampleQueue {
  int capacity() const {
    return int(data.size());
  }

  void reserve(int n) {
    data.resize(n);
  }

  void clear() {
    size = 0;
  }

  void push(const T* samples, int num_samples) {
    if (capacity() == 0) {
      return;
    }

    int end{};
    while (end < num_samples && size < capacity()) {
      data[size++] = samples[end++];
    }

    int num_rem = num_samples - end;
    while (num_rem > 0) {
      const int num_rotate = std::min(capacity(), num_rem);
      std::rotate(data.begin(), data.begin() + num_rotate, data.end());
      for (int i = 0; i < num_rotate; i++) {
        data[size - num_rotate + i] = samples[end++];
      }
      num_rem -= num_rotate;
    }
  }

  std::vector<T> data;
  int size{};
};

}