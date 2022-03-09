#include "serial_lever.hpp"
#include <string>
#include <cstring>

namespace om {

namespace {

std::optional<LeverState> parse_state(const std::string& s) {
#if 0
  printf("Source: %s\n", s.c_str());
#endif

  const auto not_found = std::string::npos;
  constexpr const char* sg = "strain gauge reading: ";
  constexpr const char* cpwm = "calculated PWM: ";
  constexpr const char* real_pwm = "acutal PWM: ";

  auto sg_it = s.find(sg);
  auto cpwm_it = s.find(cpwm);
  auto real_pwm_it = s.find(real_pwm);  //  @NOTE: typo

  if (sg_it == not_found || cpwm_it == not_found || real_pwm_it == not_found) {
    return std::nullopt;
  }

  char* ignore;
  LeverState result{};
  result.strain_gauge = std::strtof(s.data() + sg_it + std::strlen(sg), &ignore);
  result.calculated_pwm = std::strtof(s.data() + cpwm_it + std::strlen(cpwm), &ignore);
  result.actual_pwm = std::strtof(s.data() + real_pwm_it + std::strlen(real_pwm), &ignore);
  return result;
}

} //  anon

std::string to_string(const LeverState& state, const std::string& delim) {
  std::string result;
  result += "strain_gauge: " + std::to_string(state.strain_gauge);
  result += delim + "calculated_pwm: " + std::to_string(state.calculated_pwm);
  result += delim + "actual_pwm: " + std::to_string(state.actual_pwm);
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

}
