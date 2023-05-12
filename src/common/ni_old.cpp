#include "ni.hpp"
#include <NIDAQmx.h>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

namespace om {

namespace {

struct {
	TaskHandle task_handle{};
	int num_samples_per_channel{};
	int num_analog_input_channels{};

	std::thread worker_thread;
	std::atomic<bool> remote_initialized{};
	std::atomic<bool> keep_processing{};

	std::vector<double> sample_buffer;
	int latest_num_samples_acquired{};

	std::mutex read_mutex;

} globals;

void log_ni_error() {
	char err_buff[2048];
	memset(err_buff, 0, 2048);
	DAQmxGetExtendedErrorInfo(err_buff, 2048);
	printf("DAQmxError: %s\n", err_buff);
}

bool local_init(const ni::InitParams& params) {
	assert(params.num_input_channels <= 32 && "Too many channels.");

	bool success{};
	int32 status{};

	auto& task_handle = globals.task_handle;
	status = DAQmxCreateTask("", &task_handle);
	if (status != 0) {
		globals.task_handle = nullptr;
		goto error;
	}

	for (int i = 0; i < params.num_input_channels; i++) {
		auto& channel_desc = params.input_channel_params[i];
		status = DAQmxCreateAIVoltageChan(task_handle, channel_desc.channel_name, "", DAQmx_Val_Cfg_Default, channel_desc.min_value, channel_desc.max_value, DAQmx_Val_Volts, NULL);
		if (status != 0) {
			goto error;
		}

		globals.num_analog_input_channels++;
	}

	if (params.sample_clock_channel_name) {
		status = DAQmxExportSignal(globals.task_handle, DAQmx_Val_SampleClock, params.sample_clock_channel_name.value());
		if (status != 0) {
			goto error;
		}
	}

	status = DAQmxCfgSampClkTiming(task_handle, "", params.task_params->sample_rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, params.task_params->samples_per_channel);
	if (status != 0) {
		goto error;
	}


	status = DAQmxStartTask(task_handle);
	if (status != 0) {
		goto error;
	}

	globals.sample_buffer.resize(uint64_t(params.task_params->samples_per_channel) * uint64_t(params.num_input_channels));
	globals.num_samples_per_channel = params.task_params->samples_per_channel;

	success = true;
	globals.remote_initialized.store(true);

error:
	if (!success) {
		log_ni_error();
		ni::terminate_ni();
	}

	return success;
}

void remote_acquire_samples() {
	if (!globals.remote_initialized.load()) {
		return;
	}

	std::lock_guard<std::mutex> lock{globals.read_mutex};

	auto* data = globals.sample_buffer.data();
	const int sample_capacity = globals.num_samples_per_channel;

	int32 num_read{};
	int status = DAQmxReadAnalogF64(globals.task_handle, DAQmx_Val_Auto, 100.0, DAQmx_Val_GroupByChannel, data, sample_capacity, &num_read, NULL);

	if (status != 0) {
		log_ni_error();
	}

	globals.latest_num_samples_acquired = num_read;
}

void local_terminate() {
	if (globals.task_handle) {
		DAQmxStopTask(globals.task_handle);
		DAQmxClearTask(globals.task_handle);
		globals.task_handle = nullptr;
		globals.num_analog_input_channels = 0;
		globals.remote_initialized.store(false);
	}
}

void remote_thread() {
	while (globals.keep_processing.load()) {
		remote_acquire_samples();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

}	//	anon

bool ni::init_ni(const ni::InitParams& params) {
	if (!local_init(params)) {
		return false;
	}

	globals.keep_processing.store(true);
	globals.worker_thread = std::thread(remote_thread);
	return true;
}

int ni::read_analog_samples(double* data, int num_samples) {
	if (!globals.remote_initialized.load()) {
		return 0;
	}

	assert(num_samples <= globals.num_samples_per_channel && globals.num_analog_input_channels > 0);

	//	Lock has to proceed read of `latest_num_samples_acquired`
	std::lock_guard<std::mutex> lock{globals.read_mutex};
	const int num_write = std::min(num_samples, globals.latest_num_samples_acquired);
	const int total_data_size = num_write * globals.num_analog_input_channels;

	if (num_write == 0) {
		return 0;
	}

	memcpy(data, globals.sample_buffer.data(), total_data_size * sizeof(double));
	return num_write;
}

void ni::terminate_ni() {
	globals.keep_processing.store(false);

	if (globals.worker_thread.joinable()) {
		globals.worker_thread.join();
	}

	local_terminate();
}

}