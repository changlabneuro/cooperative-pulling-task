#pragma once

#include <optional>

namespace om::ni {

struct ChannelParams {
	float min_value;
	float max_value;
	const char* channel_name;
};

struct TaskParams {
	double sample_rate;
	int samples_per_channel;
};

struct InitParams {
	const TaskParams* task_params;
	const ChannelParams* input_channel_params;
	int num_input_channels;
	std::optional<const char*> sample_clock_channel_name;
};

bool init_ni(const InitParams& params);
int read_analog_samples(double* samples, int sample_count);
void terminate_ni();

}