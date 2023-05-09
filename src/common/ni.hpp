#pragma once

#include "time.hpp"
#include <vector>
#include <optional>

namespace om::ni {

struct ChannelDescriptor {
  const char* name;
  double min_value;
  double max_value;
};

struct InitParams {
  double sample_rate;
  int num_samples_per_channel;
  const ChannelDescriptor* analog_input_channels;
  int num_analog_input_channels;
  std::optional<const char*> sample_clock_channel_name;
};

struct TriggerTimePoint {
  double elapsed_time;
  uint64_t sample_index;
};

struct SampleBuffer {
  double* data;
  int num_samples_per_channel;
  int num_channels;
  uint64_t sample0_index;
  double sample0_time;
};

bool init_ni(const InitParams& params);
void terminate_ni();

int read_sample_buffers(const SampleBuffer** buffs);
void release_sample_buffers();
std::vector<TriggerTimePoint> read_trigger_time_points(om::TimePoint* t0);

}