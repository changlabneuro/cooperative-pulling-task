#pragma once

#include "serial.hpp"

namespace om {

struct LeverState {
  float strain_gauge;
  float calculated_pwm;
  float actual_pwm;
};

struct LeverContext {
  LeverState state{};
  int commanded_force_grams{10};
  int canonical_force_grams{10};
};

constexpr uint32_t default_baud_rate() {
  return 9600;
}

constexpr uint32_t default_read_write_timeout() {
  return 25;
}

std::string to_string(const LeverState& state, const std::string& delim = "\n");

std::optional<LeverState> read_state(const SerialContext& context);
std::optional<std::string> set_force_grams(const SerialContext& context, int force);

}