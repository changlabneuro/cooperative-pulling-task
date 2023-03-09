#include "serial_lever.hpp"
#include <string>
#include <cstring>

namespace om {

namespace {

std::optional<int> parse_force(const std::string& s) {
  constexpr const char* tg = "target grams: ";
  auto tg_it = s.find(tg);
  if (tg_it == std::string::npos) {
    return std::nullopt;
  } else {
    char* ignore;
    return std::strtol(s.data() + tg_it + std::strlen(tg), &ignore, 10);
  }
}

[[maybe_unused]] float parse_float(const char* base, size_t off, const char* prefix) {
  char* ignore;
  return std::strtof(base + off + std::strlen(prefix), &ignore);
}

std::optional<LeverState> parse_state(const std::string& s) {
#if 0
    printf("Source: %s\n", s.c_str());
#endif

    const auto not_found = std::string::npos;
    constexpr const char* sg = "strain gauge reading: ";
    constexpr const char* cpwm = "calculated PWM: ";
    constexpr const char* real_pwm = "acutal PWM: ";
    constexpr const char* pot_str = "P: ";

    auto sg_it = s.find(sg);
    auto cpwm_it = s.find(cpwm);
    auto real_pwm_it = s.find(real_pwm);  //  @NOTE: typo
    auto pot_it = s.find(pot_str);  //  @NOTE: typo

    if (sg_it == not_found || cpwm_it == not_found || real_pwm_it == not_found || pot_it == not_found) {
        return std::nullopt;
    }

    char* ignore;
    LeverState result{};
    result.strain_gauge = std::strtof(s.data() + sg_it + std::strlen(sg), &ignore);
    result.calculated_pwm = std::strtof(s.data() + cpwm_it + std::strlen(cpwm), &ignore);
    result.actual_pwm = std::strtof(s.data() + real_pwm_it + std::strlen(real_pwm), &ignore);
    result.potentiometer_reading = std::strtof(s.data() + pot_it + std::strlen(pot_str), &ignore);
    return result;
}

} //  anon

std::string to_string(const LeverState& state, const std::string& delim) {
  std::string result;
  result += "strain_gauge: " + std::to_string(state.strain_gauge);
  result += delim + "calculated_pwm: " + std::to_string(state.calculated_pwm);
  result += delim + "actual_pwm: " + std::to_string(state.actual_pwm);
  result += delim + "potent_reading: " + std::to_string(state.potentiometer_reading);
  return result;
}

std::optional<LeverState> read_state(const SerialContext& context) {
  context.instance->write("s");
  if (auto str = readline(context)) {
    return parse_state(str.value());
  } else {
    return std::nullopt;
  }
}

std::optional<int> set_force_grams(const SerialContext& context, int force) {
  std::string command{"g"};
  command += std::to_string(force);
  command += "\n";
  context.instance->write(command);
  if (auto res = readline(context)) {
    return parse_force(res.value());
  } else {
    return std::nullopt;
  }
}

bool set_lever_direction(const SerialContext& context, SerialLeverDirection dir) {
  const char* cmd = dir == SerialLeverDirection::Forward ? "f" : "z";
  std::string command{cmd};
  command += "\n";
  context.instance->write(command);
  if (auto res = readline(context)) {
    return true;
  } else {
    return false;
  }
}

}
