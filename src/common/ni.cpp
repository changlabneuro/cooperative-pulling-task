#include "ni.hpp"
#include "ringbuffer.hpp"
#include "common.hpp"
#include <NIDAQmx.h>
#include <vector>
#include <cassert>
#include <optional>
#include <atomic>
#include <mutex>

namespace {

using namespace om;

struct Config {
  static constexpr int input_sample_buffer_ring_buffer_capacity = 16;
  static constexpr uint64_t input_sample_index_sync_interval = 10000;
};

struct NIInputSampleSyncPoints {
  void clear() {
    time_points.clear();
  }

  std::vector<ni::TriggerTimePoint> time_points;
};

struct OutputPulseQueue {
  struct Pulse {
    int channel;
    float time_remaining;
  };

  void clear() {
    pulses.clear();
  }

  void push(int channel, float time_left) {
    for (auto& pulse : pulses) {
      if (pulse.channel == channel) {
        pulse.time_remaining = time_left;
        return;
      }
    }

    auto& pulse = pulses.emplace_back();
    pulse.channel = channel;
    pulse.time_remaining = time_left;
  }

  std::vector<Pulse> pulses;
};

struct TriggerDetect {
  double rising_threshold{1.5};
  double falling_threshold{0.25};
  bool high{};
};

struct TriggerTimePoints {
  void init(om::TimePoint t0) {
    time0 = t0;
    time_points.reserve(1000);
  }

  void clear() {
    time0 = {};
    time_points.clear();
  }

  om::TimePoint time0{};
  std::vector<ni::TriggerTimePoint> time_points;
};

struct NITriggerDetect {
  void init(om::TimePoint t0) {
    time_points.init(t0);
  }

  void reset() {
    detect = {};
    time_points.clear();
  }

  TriggerDetect detect;
  TriggerTimePoints time_points;
};

std::optional<int> detect_first_rising(
  TriggerDetect* detect, double* one_channel_data, int num_samples_in_channel) {
  //
  std::optional<int> result;
  for (int i = 0; i < num_samples_in_channel; i++) {
    double sample = one_channel_data[i];
    if (!detect->high && sample >= detect->rising_threshold) {
      if (!result) {
        result = i;
      }
      detect->high = true;

    } else if (detect->high && sample < detect->falling_threshold) {
      detect->high = false;
    }
  }

  return result;
}

void push_trigger_time_point(
  TriggerTimePoints* tps, int sample_offset, uint64_t sample0_index,
  double sample0_time, double ni_sample_rate) {
  //
  ni::TriggerTimePoint tp{};
  tp.sample_index = sample0_index + uint64_t(sample_offset);
  tp.elapsed_time = sample0_time + double(sample_offset) / ni_sample_rate;
  tps->time_points.push_back(tp);
}

struct StaticSampleBufferArray {
  void clear() {
    num_buffers = 0;
  }

  void push(ni::SampleBuffer buff) {
    assert(num_buffers < Config::input_sample_buffer_ring_buffer_capacity);
    buffers[num_buffers++] = buff;
  }

  std::optional<ni::SampleBuffer> pop_back() {
    if (num_buffers > 0) {
      return buffers[--num_buffers];
    } else {
      return std::nullopt;
    }
  }

  ni::SampleBuffer buffers[Config::input_sample_buffer_ring_buffer_capacity]{};
  int num_buffers{};
};

struct NITask {
  TaskHandle task{ nullptr };
  bool started{};
};

struct {
  NITask ni_input_export_task{};

  NITriggerDetect ni_trigger_detect{};
  std::mutex ni_trigger_time_point_mutex{};

  std::vector<double> daq_sample_buffer;
  int num_samples_per_input_channel{};
  int num_analog_input_channels{};
  double input_sample_rate{};
  uint64_t ni_num_input_samples_acquired{};

  NITask ni_analog_output_tasks[32]{};
  ni::ChannelDescriptor analog_output_channel_descs[32]{};
  int num_analog_output_channels{};

  RingBuffer<ni::SampleBuffer, Config::input_sample_buffer_ring_buffer_capacity> send_to_ni_daq;
  StaticSampleBufferArray received_from_ni{};

  RingBuffer<ni::SampleBuffer, Config::input_sample_buffer_ring_buffer_capacity> send_from_ni_daq;
  StaticSampleBufferArray available_to_send_to_ui{};

  std::vector<std::unique_ptr<double[]>> sample_buffer_data;
  om::TimePoint time0{};
  bool initialized{};

  OutputPulseQueue output_pulse_queue;
  om::TimePoint last_time{};
  bool first_time{};

  NIInputSampleSyncPoints input_sample_sync_points;

} globals;

void log_ni_error() {
  char err_buff[2048];
  memset(err_buff, 0, 2048);
  DAQmxGetExtendedErrorInfo(err_buff, 2048);
  printf("DAQmxError: %s\n", err_buff);
}

void ni_trigger_detect(
  NITriggerDetect* detect, uint64_t sample0_index, double sample0_time, double* detect_channel,
  int num_samples, double sample_rate) {
  //
  if (auto ind = detect_first_rising(&detect->detect, detect_channel, num_samples)) {
    push_trigger_time_point(
      &detect->time_points, ind.value(), sample0_index, sample0_time, sample_rate);
  }
}

[[nodiscard]] uint32_t ni_read_data(TaskHandle task, double* read_buff, uint32_t num_samples) {
  int32 num_read{};
  const int32 status = DAQmxReadAnalogF64(
    task, num_samples, 100.0, DAQmx_Val_GroupByChannel,
    read_buff, num_samples, &num_read, nullptr);
  if (status != 0) {
    log_ni_error();
  }

  assert(num_read <= num_samples);
  return uint32_t(num_read);
}

void ni_acquire_sample_buffers() {
  const int num_rcv = globals.send_to_ni_daq.size();
  for (int i = 0; i < num_rcv; i++) {
    globals.available_to_send_to_ui.push(globals.send_to_ni_daq.read());
  }
}

void ni_maybe_send_sample_buffer(
  const double* read_buff, uint32_t num_samples, uint64_t sample0_index, double sample0_time) {
  //
  if (globals.send_from_ni_daq.full()) {
    return;
  }

  const uint32_t num_channels = globals.num_analog_input_channels;
  if (auto opt_send = globals.available_to_send_to_ui.pop_back()) {
    auto& send = opt_send.value();
    const uint32_t tot_data_size = num_samples * num_channels;

    memcpy(send.data, read_buff, sizeof(double) * tot_data_size);
    send.num_samples_per_channel = num_samples;
    send.num_channels = num_channels;
    send.sample0_time = sample0_time;
    send.sample0_index = sample0_index;

    globals.send_from_ni_daq.write(send);
  }
}

int32 CVICALLBACK ni_input_sample_callback(TaskHandle task, int32, uInt32 num_samples, void*) {
  //
  assert(num_samples == globals.num_samples_per_input_channel);
  assert(globals.daq_sample_buffer.size() == num_samples * globals.num_analog_input_channels);

  ni_acquire_sample_buffers();

  double* read_buff = globals.daq_sample_buffer.data();
  const uint32_t num_read = ni_read_data(task, read_buff, num_samples);

  const uint64_t sample0_index = globals.ni_num_input_samples_acquired;
  double sample0_time = elapsed_time(globals.time0, now());

  //  Look for rising edges in analog voltage trace on the first channel.
  //  @TODO: We could specify a choice of channel index on which to look.
  {
    std::lock_guard<std::mutex> lock(globals.ni_trigger_time_point_mutex);

    ni_trigger_detect(
      &globals.ni_trigger_detect, sample0_index, sample0_time,
      read_buff, num_read, globals.input_sample_rate);
  }

  ni_maybe_send_sample_buffer(read_buff, num_read, sample0_index, sample0_time);
  globals.ni_num_input_samples_acquired += uint64_t(num_read);

  return 0;
}

void init_input_data_handoff(int num_channels, int num_samples_per_channel) {
  assert(globals.sample_buffer_data.empty());

  const int total_num_samples = num_channels * num_samples_per_channel;
  globals.daq_sample_buffer.resize(total_num_samples);

  for (int i = 0; i < Config::input_sample_buffer_ring_buffer_capacity - 1; i++) {
    //  - 1 because ring buffer capacity is actually one less than
    //  `input_sample_buffer_ring_buffer_capacity`
    auto& dst = globals.sample_buffer_data.emplace_back();
    dst = std::make_unique<double[]>(total_num_samples);

    ni::SampleBuffer buff{};
    buff.data = dst.get();
    if (!globals.send_to_ni_daq.maybe_write(buff)) {
      assert(false);
    }
  }
}

bool start_outputs(const ni::InitParams& params) {
  globals.num_analog_output_channels = params.num_analog_output_channels;

  for (int i = 0; i < params.num_analog_output_channels; i++) {
    int32 status{};

    auto& task_handle = globals.ni_analog_output_tasks[i].task;
    auto& channel_desc = params.analog_output_channels[i];

    globals.analog_output_channel_descs[i] = channel_desc;

    std::string task_name{ "OutputTask" };
    task_name += std::to_string(i);
    status = DAQmxCreateTask(task_name.c_str(), &task_handle);
    if (status != 0) {
      log_ni_error();
      return false;
    }

    status = DAQmxCreateAOVoltageChan(task_handle, channel_desc.name, "", channel_desc.min_value, channel_desc.max_value, DAQmx_Val_Volts, NULL);
    if (status != 0) {
      log_ni_error();
      return false;
    }

    status = DAQmxCfgSampClkTiming(task_handle, "", params.sample_rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, params.num_samples_per_channel);
    if (status != 0) {
      log_ni_error();
      return false;
    }

    status = DAQmxStartTask(task_handle);
    if (status != 0) {
      log_ni_error();
      return false;
    } else {
      globals.ni_analog_output_tasks[i].started = true;
    }
  }

  return true;
}

bool start_inputs_and_exports(const ni::InitParams& params) {
  const uint32_t num_samples = params.num_samples_per_channel;

  auto& task_handle = globals.ni_input_export_task.task;
  int32 status{};

  status = DAQmxCreateTask("Task0", &task_handle);
  if (status != 0) {
    log_ni_error();
    return false;
  }

  for (int i = 0; i < params.num_analog_input_channels; i++) {
    auto& channel_desc = params.analog_input_channels[i];
    status = DAQmxCreateAIVoltageChan(task_handle, channel_desc.name, "", DAQmx_Val_Cfg_Default, channel_desc.min_value, channel_desc.max_value, DAQmx_Val_Volts, NULL);
    if (status != 0) {
      log_ni_error();
      return false;
    }
  }

  if (params.sample_clock_channel_name) {
    status = DAQmxExportSignal(task_handle, DAQmx_Val_SampleClock, params.sample_clock_channel_name.value());
    if (status != 0) {
      log_ni_error();
      return false;
    }
  }

  status = DAQmxCfgSampClkTiming(task_handle, "", params.sample_rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, params.num_samples_per_channel);
  if (status != 0) {
    log_ni_error();
    return false;
  }

  status = DAQmxRegisterEveryNSamplesEvent(
    task_handle, DAQmx_Val_Acquired_Into_Buffer, num_samples, 0, ni_input_sample_callback, nullptr);
  if (status != 0) {
    log_ni_error();
    return false;
  }

  status = DAQmxStartTask(task_handle);
  if (status != 0) {
    log_ni_error();
    return false;
  }
  else {
    globals.ni_input_export_task.started = true;
  }

  return true;
}

void clear_task(NITask* task) {
  if (task->started) {
    DAQmxStopTask(task->task);
    task->started = false;
  }
  if (task->task) {
    DAQmxClearTask(task->task);
    task->task = nullptr;
  }
}

bool start_daq(const ni::InitParams& params) {
  if (!start_inputs_and_exports(params)) {
    return false;
  }

  if (!start_outputs(params)) {
    return false;
  }

  return true;
}

void stop_daq() {
  clear_task(&globals.ni_input_export_task);
  for (int i = 0; i < globals.num_analog_output_channels; i++) {
    clear_task(&globals.ni_analog_output_tasks[i]);
  }
}

bool analog_write(int channel, float v) {
  assert(channel >= 0 && channel < globals.num_analog_output_channels);
  auto& ni_task = globals.ni_analog_output_tasks[channel];
  if (!ni_task.started) {
    return false;
  }

  auto& channel_desc = globals.analog_output_channel_descs[channel];
  double val = clamp(double(v), channel_desc.min_value, channel_desc.max_value);
  int32 status = DAQmxWriteAnalogScalarF64(ni_task.task, false, -1.0, val, nullptr);
  if (status != 0) {
    log_ni_error();
    return false;
  }

  return true;
}

} //  anon

namespace om {

bool ni::init_ni(const InitParams& params) {
  if (globals.initialized) {
    terminate_ni();
  }

  const auto t0 = om::now();
  globals.time0 = t0;
  globals.ni_trigger_detect.init(t0);

  globals.input_sample_rate = params.sample_rate;
  globals.num_analog_input_channels = params.num_analog_input_channels;
  globals.num_samples_per_input_channel = params.num_samples_per_channel;

  init_input_data_handoff(params.num_analog_input_channels, params.num_samples_per_channel);

  if (!start_daq(params)) {
    terminate_ni();
    return false;
  } else {
    globals.initialized = true;
    return true;
  }
}

void ni::update_ni() {
  release_sample_buffers();

  if (globals.first_time) {
    globals.last_time = now();
    globals.first_time = false;
  }

  auto curr_t = now();
  double dt = elapsed_time(globals.last_time, curr_t);
  globals.last_time = curr_t;

  { //  set high terminals -> low
    auto pulse_it = globals.output_pulse_queue.pulses.begin();
    while (pulse_it != globals.output_pulse_queue.pulses.end()) {
      pulse_it->time_remaining = std::max(0.0, pulse_it->time_remaining - dt);
      if (pulse_it->time_remaining == 0.0) {
        analog_write(pulse_it->channel, 0.0f);
        pulse_it = globals.output_pulse_queue.pulses.erase(pulse_it);
      }
      else {
        ++pulse_it;
      }
    }
  }

  { //  sync between ni sample indices and task time
    const SampleBuffer* buffs{};
    int num_buffs = std::min(1, read_sample_buffers(&buffs));
    if (num_buffs > 0) {
      auto& buff = buffs[0];
      bool push_timepoint{};
      if (globals.input_sample_sync_points.time_points.empty()) {
        push_timepoint = true;
      }
      else {
        auto& last_timepoint = globals.input_sample_sync_points.time_points.back();
        if (buff.sample0_index - last_timepoint.sample_index >= Config::input_sample_index_sync_interval) {
          push_timepoint = true;
        }
      }

      if (push_timepoint) {
        auto& next_sync_point = globals.input_sample_sync_points.time_points.emplace_back();
        next_sync_point.sample_index = buff.sample0_index;
        next_sync_point.elapsed_time = buff.sample0_time;
      }
    }
  }
}

void ni::terminate_ni() {
  stop_daq();
  globals.daq_sample_buffer.clear();
  globals.ni_trigger_detect.reset();
  globals.num_samples_per_input_channel = 0;
  globals.num_analog_input_channels = 0;
  globals.num_analog_output_channels = 0;
  globals.input_sample_rate = 0.0;
  globals.ni_num_input_samples_acquired = 0;
  globals.send_to_ni_daq.clear();
  globals.send_from_ni_daq.clear();
  globals.received_from_ni.clear();
  globals.available_to_send_to_ui.clear();
  globals.sample_buffer_data.clear();
  globals.time0 = {};
  globals.output_pulse_queue.clear();
  globals.input_sample_sync_points.clear();
  globals.initialized = false;
}

int ni::read_sample_buffers(const SampleBuffer** buffs) {
  int num_rcv = globals.send_from_ni_daq.size();
  for (int i = 0; i < num_rcv; i++) {
    globals.received_from_ni.push(globals.send_from_ni_daq.read());
  }

  *buffs = globals.received_from_ni.buffers;
  return globals.received_from_ni.num_buffers;
}

void ni::release_sample_buffers() {
  while (true) {
    auto buff = globals.received_from_ni.pop_back();
    if (buff) {
      if (!globals.send_to_ni_daq.maybe_write(std::move(buff.value()))) {
        assert(false);
      }
    } else {
      break;
    }
  }
}

om::TimePoint ni::read_time0() {
  return globals.time0;
}

std::vector<ni::TriggerTimePoint> ni::read_sync_time_points() {
  return globals.input_sample_sync_points.time_points;
}

std::vector<ni::TriggerTimePoint> ni::read_trigger_time_points() {
  std::vector<ni::TriggerTimePoint> tps;
  {
    std::lock_guard<std::mutex> lock(globals.ni_trigger_time_point_mutex);
    tps = globals.ni_trigger_detect.time_points.time_points;
  }
  return tps;
}

bool ni::write_analog_pulse(int channel, float val, float for_time) {
  if (!analog_write(channel, val)) {
    return false;
  }

  globals.output_pulse_queue.push(channel, for_time);
  return true;
}

} //  om
