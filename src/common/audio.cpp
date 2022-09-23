#include "audio.hpp"
#include "ringbuffer.hpp"
#include "common.hpp"
#include "AudioFile/AudioFile.h"
#include "portaudio.h"
#include <cassert>
#include <memory>
#include <unordered_map>
#include <iostream>

namespace om::audio {

constexpr int max_num_buffer_output_channels = 4;

struct Buffer {
  std::unique_ptr<float[]> data;
  double sample_rate;
  int channels;
  int frames;
};

struct PushBuffer {
  uint32_t handle_id;
  Buffer* buffer;
};

struct PendingPlayingBuffer {
  BufferHandle buffer;
  float gain[max_num_buffer_output_channels];
};

struct PlayingBuffer {
  Buffer* buffer;
  double frame;
  float gain[max_num_buffer_output_channels];
};

struct PlayingBuffers {
  static constexpr int max_num_playing_buffers = 2048;

  PlayingBuffer buffers[max_num_playing_buffers]{};
  int num_playing_buffers{};
};

namespace {

struct {
  bool pa_initialized{};
  bool pa_stream_started{};

  int num_output_channels{2};
  int frames_per_buffer{1024};
  double sample_rate{44.1e3};
  PaStream* stream{};

  std::unordered_map<uint32_t, Buffer*> render_buffers;
  std::unordered_map<uint32_t, std::unique_ptr<Buffer>> main_buffers;
  RingBuffer<PushBuffer, 1024> push_buffers;
  uint32_t next_buffer_id{1};

  RingBuffer<PendingPlayingBuffer, 1024> pending_play;
  PlayingBuffers playing;
} globals;

float interp_sample(Buffer* buff, uint64_t i0, uint64_t i1, double frame, int channel) {
  assert(i0 < uint64_t(buff->frames) && i1 < uint64_t(buff->frames));
  return lerp(
    float(frame - double(i0)),
    buff->data[i0 * buff->channels + channel],
    buff->data[i1 * buff->channels + channel]);
}

void play_buffers(PlayingBuffers* buffs, float* out) {
  int buff_ind{};
  const int num_playing = buffs->num_playing_buffers;
  for (int i = 0; i < num_playing; i++) {
    auto& buff = buffs->buffers[buff_ind];

    bool elapsed{};
    for (int s = 0; s < globals.frames_per_buffer; s++) {
      auto i0 = uint64_t(buff.frame);
      auto i1 = std::min(i0 + 1, uint64_t(buff.buffer->frames - 1));

      float samples[2];
      samples[0] = interp_sample(buff.buffer, i0, i1, buff.frame, 0);
      if (globals.num_output_channels > 1 && buff.buffer->channels > 1) {
        samples[1] = interp_sample(buff.buffer, i0, i1, buff.frame, 1);
      } else {
        samples[1] = samples[0];
      }

      for (int j = 0; j < std::min(2, globals.num_output_channels); j++) {
        assert(j < max_num_buffer_output_channels);
        out[s * globals.num_output_channels + j] += samples[j] * buff.gain[j];
      }

      buff.frame += buff.buffer->sample_rate / globals.sample_rate;
      elapsed = buff.frame >= double(buff.buffer->frames);
      if (elapsed) {
        break;
      }
    }

    if (elapsed) {
      std::rotate(
        buffs->buffers + buff_ind,
        buffs->buffers + buff_ind + 1,
        buffs->buffers + buffs->num_playing_buffers--);
    } else {
      ++buff_ind;
    }
  }
}

bool push_playing(PlayingBuffers* buffs, const PendingPlayingBuffer& pend) {
  if (buffs->num_playing_buffers == PlayingBuffers::max_num_playing_buffers) {
    return false;
  } else {
    auto it = globals.render_buffers.find(pend.buffer.id);
    if (it == globals.render_buffers.end()) {
      assert(false);
      return false;
    } else {
      PlayingBuffer playing{};
      for (int i = 0; i < max_num_buffer_output_channels; i++) {
        playing.gain[i] = pend.gain[i];
      }
      playing.buffer = it->second;
      buffs->buffers[buffs->num_playing_buffers++] = playing;
      return true;
    }
  }
}

int stream_callback(const void*, void* output_buffer,
                    unsigned long, const PaStreamCallbackTimeInfo*,
                    unsigned long, void*) {
  {
    int num_buffs = globals.push_buffers.size();
    for (int i = 0; i < num_buffs; i++) {
      auto buff = globals.push_buffers.read();
      globals.render_buffers[buff.handle_id] = buff.buffer;
    }
  }
  {
    int num_buffs = globals.pending_play.size();
    for (int i = 0; i < num_buffs; i++) {
      auto pend = globals.pending_play.read();
      push_playing(&globals.playing, pend);
    }
  }

  auto* out = static_cast<float*>(output_buffer);
  std::fill(out, out + globals.frames_per_buffer * globals.num_output_channels, 0.0f);
  play_buffers(&globals.playing, out);
  return 0;
}

} //  anon

void init_audio() {
  assert(!globals.pa_initialized);
  auto err = Pa_Initialize();
  assert(err == paNoError);
  globals.pa_initialized = true;

  PaStream* stream{};
  err = Pa_OpenDefaultStream(
    &stream, 0, globals.num_output_channels,
    paFloat32, globals.sample_rate, globals.frames_per_buffer, stream_callback, nullptr);
  if (err != paNoError) {
    std::cerr << "Failed to open default audio stream." << std::endl;
    return;
  }

  err = Pa_StartStream(stream);
  assert(err == paNoError);
  globals.pa_stream_started = true;
}

void terminate_audio() {
  if (globals.pa_initialized) {
    if (globals.pa_stream_started) {
      Pa_StopStream(globals.stream);
      Pa_CloseStream(globals.stream);
      globals.pa_stream_started = false;
    }
    Pa_Terminate();
    globals.pa_initialized = false;
  }
}

std::optional<BufferHandle> create_buffer(const float* data, double sr, int channels, int frames) {
  assert(channels > 0 && frames > 0);
  if (!globals.pa_stream_started || globals.push_buffers.full()) {
    return std::nullopt;
  }

  Buffer buff{};
  buff.sample_rate = sr;
  buff.channels = channels;
  buff.frames = frames;
  buff.data = std::make_unique<float[]>(channels * frames);
  memcpy(buff.data.get(), data, channels * frames * sizeof(float));

  BufferHandle result{globals.next_buffer_id++};
  auto res_buff = std::make_unique<Buffer>(std::move(buff));
  auto* buff_ptr = res_buff.get();
  globals.main_buffers[result.id] = std::move(res_buff);

  PushBuffer push{};
  push.buffer = buff_ptr;
  push.handle_id = result.id;
  globals.push_buffers.write(push);
  return result;
}

std::optional<BufferHandle> read_buffer(const char* filepath) {
  static std::unordered_map<std::string, BufferHandle> handle_cache;

  if (auto it = handle_cache.find(std::string{filepath}); it != handle_cache.end()) {
    return it->second;
  }

  AudioFile<float> file;

  try {
    file.load(filepath);
  } catch (...) {
    return std::nullopt;
  }

  const int num_channels = file.getNumChannels();
  assert(num_channels <= max_num_buffer_output_channels);

  const int spc = file.getNumSamplesPerChannel();
  std::vector<float> data(num_channels * spc);
  if (data.empty()) {
    return std::nullopt;
  }

  for (int i = 0; i < spc; i++) {
    for (int j = 0; j < num_channels; j++) {
      data[i * num_channels + j] = file.samples[j][i];
    }
  }

  auto res = create_buffer(data.data(), file.getSampleRate(), num_channels, spc);
  if (res) {
    handle_cache[std::string{filepath}] = res.value();
  }
  return res;
}

namespace {

bool play_buffer(BufferHandle buff, float gain_l, float gain_r) {
  assert(globals.pa_stream_started);
  if (globals.pending_play.full()) {
    assert(false);
    return false;
  } else {
    PendingPlayingBuffer pend{};
    pend.buffer = buff;
    pend.gain[0] = gain_l;
    pend.gain[1] = gain_r;
    globals.pending_play.write(pend);
    return true;
  }
}

bool play_buffer_channel_l(BufferHandle buff, float gain) {
  return play_buffer(buff, gain, 0.0f);
}

bool play_buffer_channel_r(BufferHandle buff, float gain) {
  return play_buffer(buff, 0.0f, gain);
}

} //  anon

bool play_buffer_both(BufferHandle buff, float gain) {
  return play_buffer(buff, gain, gain);
}

bool play_buffer_on_channel(BufferHandle buff, int channel, float gain) {
  //  @TODO: Could set gain in PendingPlayingBuffer directly.
  assert(channel >= 0 && channel < 2);
  if (channel == 0) {
    return play_buffer_channel_l(buff, gain);
  } else {
    return play_buffer_channel_r(buff, gain);
  }
}

}
