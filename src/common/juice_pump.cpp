#include "juice_pump.hpp"
#include "serial.hpp"
#include "ringbuffer.hpp"
#include <cassert>
#include <thread>
#include <mutex>
#include <iostream>

namespace om {

namespace {

struct Config {
  static constexpr uint32_t serial_baud_rate = 19200;
  static constexpr uint32_t serial_timeout = 1000;
  static constexpr char serial_terminator = '\r';
  static constexpr int max_num_pumps = 4;
};

enum class PumpCommandType {
  SetRate,
  SetVolume,
  SetAddress,
  RunProgram,
  StopProgram
};

struct PumpCommandData {
  struct SetRate {
    int rate;
    pump::RateUnits units;
  };

  struct SetVolume {
    float volume;
    pump::VolumeUnits units;
  };

  struct SetAddress {
    int address;
  };
};

struct PumpCommand {
  pump::PumpHandle pump;
  PumpCommandType type;
  union {
    PumpCommandData::SetRate set_rate;
    PumpCommandData::SetVolume set_volume;
    PumpCommandData::SetAddress set_address;
  };
};

PumpCommand make_run_program_command(pump::PumpHandle pump) {
  PumpCommand result{};
  result.pump = pump;
  result.type = PumpCommandType::RunProgram;
  return result;
}

PumpCommand make_stop_program_command(pump::PumpHandle pump) {
  PumpCommand result{};
  result.pump = pump;
  result.type = PumpCommandType::StopProgram;
  return result;
}

PumpCommand make_set_rate_command(pump::PumpHandle pump, int rate, pump::RateUnits units) {
  PumpCommand result{};
  result.pump = pump;
  result.type = PumpCommandType::SetRate;
  result.set_rate = {};
  result.set_rate.units = units;
  result.set_rate.rate = rate;
  return result;
}

PumpCommand make_set_volume_command(pump::PumpHandle pump, float vol, pump::VolumeUnits units) {
  PumpCommand result{};
  result.pump = pump;
  result.type = PumpCommandType::SetVolume;
  result.set_volume = {};
  result.set_volume.units = units;
  result.set_volume.volume = vol;
  return result;
}

PumpCommand make_set_address_command(pump::PumpHandle pump, int addr) {
  PumpCommand result{};
  result.pump = pump;
  result.type = PumpCommandType::SetAddress;
  result.set_address = {};
  result.set_address.address = addr;
  return result;
}

std::string make_set_rate_command_string(int addr, int rate, std::optional<pump::RateUnits> units) {
  std::string result;
  result += std::to_string(addr);
  result += " RAT ";
  result += std::to_string(rate);
  if (units) {
    result += " ";
    switch (units.value()) {
      case pump::RateUnits::mLPerHour: {
        result += "MH";
        break;
      }
      default: {
        assert(false);
      }
    }
  }
  result += Config::serial_terminator;
  return result;
}

std::string to_float_limit_trailing_digits(float v) {
  char buff[128];
  int cx = std::snprintf(buff, 128, "%0.3f", v);
  if (cx >= 0 && cx < 128) {
    return std::string{buff};
  } else {
    assert(false);
    return "";
  }
}

std::string make_set_volume_command_string(int addr, float vol,
                                           std::optional<pump::VolumeUnits> units) {
  std::string result;
  result += std::to_string(addr);
  result += " VOL ";
  result += to_float_limit_trailing_digits(vol);
#if 0
  if (units) {
    result += " ";
    switch (units.value()) {
      case pump::VolumeUnits::mL: {
        result += "ML";
        break;
      }
      default: {
        assert(false);
      }
    }
  }
#else
  (void) units;
#endif
  result += Config::serial_terminator;
  return result;
}

std::string make_run_program_command_string(int addr) {
  auto result = std::to_string(addr) + " RUN";
  result += Config::serial_terminator;
  return result;
}

std::string make_stop_program_command_string(int addr) {
  auto result = std::to_string(addr) + " STP";
  result += Config::serial_terminator;
  return result;
}

std::optional<std::string> command_to_string(const pump::PumpState& state, const PumpCommand& cmd) {
  switch (cmd.type) {
    case PumpCommandType::SetRate: {
      auto& set_rate = cmd.set_rate;
      return make_set_rate_command_string(state.address, set_rate.rate, set_rate.units);
    }
    case PumpCommandType::SetVolume: {
      auto& set_vol = cmd.set_volume;
      return make_set_volume_command_string(state.address, set_vol.volume, set_vol.units);
    }
    case PumpCommandType::RunProgram: {
      return make_run_program_command_string(state.address);
    }
    case PumpCommandType::StopProgram: {
      return make_stop_program_command_string(state.address);
    }
    default: {
      return std::nullopt;
    }
  }
}

void apply_command(pump::PumpState& state, const PumpCommand& cmd) {
  switch (cmd.type) {
    case PumpCommandType::SetRate: {
      state.rate = cmd.set_rate.rate;
      state.rate_units = cmd.set_rate.units;
      break;
    }
    case PumpCommandType::SetVolume: {
      state.volume = cmd.set_volume.volume;
      state.volume_units = cmd.set_volume.units;
      break;
    }
    case PumpCommandType::SetAddress: {
      state.address = cmd.set_address.address;
      break;
    }
    case PumpCommandType::RunProgram:
    case PumpCommandType::StopProgram: {
      //  Nothing to do.
      break;
    }
    default: {
      assert(false);
    }
  }
}

struct {
  bool initialized{};
  int num_pumps{};
  std::optional<SerialContext> open_context;

  std::array<pump::PumpState, Config::max_num_pumps> desired_pump_state{};
  std::array<pump::PumpState, Config::max_num_pumps> canonical_pump_state{};

  RingBuffer<PumpCommand, 1024> commands_to_pump;
  std::vector<PumpCommand> pending_commands_to_pump;

  std::vector<PumpCommand> pending_commands_to_execute;
  std::thread worker_thread;
  std::atomic<bool> keep_processing{};
  std::mutex canonical_pump_state_mutex;

} global_data;

void push_pending_command(PumpCommand cmd) {
  global_data.pending_commands_to_pump.push_back(cmd);
}

void apply_to_desired_state(pump::PumpHandle pump, const PumpCommand& cmd) {
  assert(pump.index < uint32_t(Config::max_num_pumps));
  apply_command(global_data.desired_pump_state[pump.index], cmd);
}

pump::PumpState* worker_read_canonical_pump_state(pump::PumpHandle pump) {
  assert(pump.index < uint32_t(Config::max_num_pumps));
  return &global_data.canonical_pump_state[pump.index];
}

void worker_execute_commands(const SerialContext& context) {
  for (auto& cmd : global_data.pending_commands_to_execute) {
    auto* state = worker_read_canonical_pump_state(cmd.pump);
    {
      std::lock_guard<std::mutex> lock(global_data.canonical_pump_state_mutex);
      apply_command(*state, cmd);
    }
    if (auto cmd_str = command_to_string(*state, cmd)) {
      context.instance->write(cmd_str.value());
    }
  }
}

void set_connection_open(int num_pumps, bool open) {
  std::lock_guard<std::mutex> lock(global_data.canonical_pump_state_mutex);
  for (int i = 0; i < num_pumps; i++) {
    global_data.canonical_pump_state[i].connection_open = open;
  }
}

void worker(std::string port, int num_pumps) {
  bool connection_open{};
  if (auto ctx = make_context(port, Config::serial_baud_rate, Config::serial_timeout)) {
    global_data.open_context = std::move(ctx.value());
    connection_open = true;
  } else {
    std::cerr << "Failed to open serial context on port: " << port << std::endl;
  }

  set_connection_open(num_pumps, connection_open);

  while (global_data.keep_processing.load()) {
    int num_commands = global_data.commands_to_pump.size();
    auto& pending_exec = global_data.pending_commands_to_execute;
    for (int i = 0; i < num_commands; i++) {
      auto cmd = global_data.commands_to_pump.read();
      pending_exec.push_back(cmd);
    }

    if (global_data.open_context) {
      worker_execute_commands(global_data.open_context.value());
      pending_exec.clear();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  global_data.open_context = std::nullopt;
  set_connection_open(num_pumps, false);
}

} //  anon

int pump::num_initialized_pumps() {
  return global_data.num_pumps;
}

pump::PumpHandle pump::ith_pump(int i) {
  return pump::PumpHandle{uint32_t(i)};
}

void pump::initialize_pump_system(std::string port, int num_pumps) {
  assert(num_pumps < Config::max_num_pumps);

  if (global_data.initialized) {
    terminate_pump_system();
  }

  global_data.num_pumps = num_pumps;

  assert(!global_data.keep_processing.load());
  global_data.keep_processing.store(true);
  global_data.worker_thread = std::thread(worker, std::move(port), num_pumps);
  global_data.initialized = true;

  for (int i = 0; i < num_pumps; i++) {
    pump::set_address_rate_volume(pump::PumpHandle{uint32_t(i)}, global_data.desired_pump_state[i]);
  }
}

void pump::terminate_pump_system() {
  if (global_data.worker_thread.joinable()) {
    global_data.keep_processing.store(false);
    global_data.worker_thread.join();
  } else {
    assert(!global_data.keep_processing.load());
  }

  global_data.initialized = false;
  global_data.num_pumps = 0;
}

void pump::set_dispensed_volume(PumpHandle pump, float vol, VolumeUnits units) {
  auto cmd = make_set_volume_command(pump, vol, units);
  apply_to_desired_state(pump, cmd);
  push_pending_command(cmd);
}

void pump::set_pump_rate(PumpHandle pump, int rate, RateUnits units) {
  auto cmd = make_set_rate_command(pump, rate, units);
  apply_to_desired_state(pump, cmd);
  push_pending_command(cmd);
}

void pump::set_address(PumpHandle pump, int address) {
  auto cmd = make_set_address_command(pump, address);
  apply_to_desired_state(pump, cmd);
  push_pending_command(cmd);
}

void pump::run_dispense_program(PumpHandle pump){
  auto cmd = make_run_program_command(pump);
  apply_to_desired_state(pump, cmd);
  push_pending_command(cmd);
}

void pump::stop_dispense_program(PumpHandle pump) {
  auto cmd = make_stop_program_command(pump);
  apply_to_desired_state(pump, cmd);
  push_pending_command(cmd);
}

void pump::set_address_rate_volume(PumpHandle pump, PumpState state) {
  set_address(pump, state.address);
  set_pump_rate(pump, state.rate, state.rate_units);
  set_dispensed_volume(pump, state.volume, state.volume_units);
}

void pump::submit_commands() {
  auto& pend = global_data.pending_commands_to_pump;
  auto it = pend.begin();
  while (it != pend.end()) {
    auto& dst = global_data.commands_to_pump;
    if (dst.maybe_write(*it)) {
      it = pend.erase(it);
    } else {
      break;
    }
  }
}

pump::PumpState pump::read_desired_pump_state(PumpHandle pump) {
  assert(pump.index < uint32_t(Config::max_num_pumps));
  return global_data.desired_pump_state[pump.index];
}

pump::PumpState pump::read_canonical_pump_state(PumpHandle pump) {
  assert(pump.index < uint32_t(Config::max_num_pumps));
  pump::PumpState result;
  {
    std::lock_guard<std::mutex> lock(global_data.canonical_pump_state_mutex);
    result = global_data.canonical_pump_state[pump.index];
  }
  return result;
}

}