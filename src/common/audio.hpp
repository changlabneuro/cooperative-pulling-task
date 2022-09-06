#pragma once

#include "identifier.hpp"
#include <cstdint>
#include <optional>

namespace om::audio {

struct BufferHandle {
  OM_INTEGER_IDENTIFIER_EQUALITY(BufferHandle, id)
  uint32_t id;
};

void init_audio();
void terminate_audio();
std::optional<BufferHandle> create_buffer(const float* data, double sr, int channels, int frames);
std::optional<BufferHandle> read_buffer(const char* file_path);
bool play_buffer(BufferHandle buff, float gain);
bool play_buffer(BufferHandle buff, float gain_l, float gain_r);
bool play_buffer_channel_l(BufferHandle buff, float gain);
bool play_buffer_channel_r(BufferHandle buff, float gain);

}