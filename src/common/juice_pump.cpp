#include "juice_pump.hpp"
#include "serial.hpp"
#include <cassert>

namespace om {

namespace {

constexpr uint32_t serial_baud_rate() {
  return 19200;
}

constexpr uint32_t serial_timeout() {
  return 1000;
}

constexpr int max_num_pumps() {
  return 4;
}

enum class RateUnits {
  MLPerHour
};

enum class VolumeUnits {
  ML,
};

struct PumpState {
  float syringe_diameter;
  int rate;
  RateUnits rate_units;
  float volume;
  VolumeUnits volume_units;
};

struct Pump {
  PumpState state;
};

struct {
  bool initialized{};
  int num_pumps{};
  std::optional<SerialContext> open_context;
  Pump pumps[max_num_pumps()];
} global_data;

} //  anon

void pump::initialize_pump_system(int num_pumps) {
  assert(!global_data.initialized && num_pumps < max_num_pumps());

  for (int i = 0; i < num_pumps; i++) {
    global_data.pumps[i] = {};
  }

  global_data.num_pumps = num_pumps;
  global_data.initialized = true;
}

void pump::terminate_pump_system() {
  close_connection();
  global_data.initialized = false;
  global_data.num_pumps = 0;
}

bool pump::is_connection_open() {
  return global_data.open_context.has_value();
}

bool pump::close_connection() {
  if (global_data.open_context) {
    global_data.open_context = std::nullopt;
    return true;
  } else {
    return false;
  }
}

bool pump::open_connection(const std::string& port) {
  if (global_data.open_context) {
    assert(false);
    return false;
  } else {
    auto ctx = make_context(port, serial_baud_rate(), serial_timeout());
    if (ctx) {
      global_data.open_context = std::move(ctx.value());
      return true;
    } else {
      return false;
    }
  }
}

}