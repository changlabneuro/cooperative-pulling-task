#pragma once

#include "time.hpp"
#include <optional>

namespace om::lever {

struct PullDetect {
  bool is_high;
  float rising_edge;
  float falling_edge;
};

struct PullDetectParams {
  float current_position;
};

struct PullDetectResult {
  bool pulled_lever;
  bool released_lever;
};

struct AutomatedPull {
  enum class ForceTransitionState {
    Idle,
    Transitioning,
    Timeout
  };

  enum class State {
    Idle = 0,
    Forwards0,
    Reverse0,
    Reverse1,
    Forwards1
  };

  ForceTransitionState force_state;
  State state;
  float current_force;
  bool target_high;
  std::optional<om::TimePoint> state_last_time;
  om::Duration state_elapsed_time;
};

struct AutomatedPullParams {
  float force_slope_g_s{800.0f};
  float force_target_low{};
  float force_target_high{100.0f};
  float force_transition_timeout_s{0.125f};
};

struct AutomatedPullResult {
  bool elapsed;
  bool active;
  std::optional<bool> set_direction;
  std::optional<float> set_force;
};

PullDetectResult detect_pull(PullDetect* pd, const PullDetectParams& params);

void start_automated_pull(AutomatedPull* pull, float current_force);
AutomatedPullResult update_automated_pull(AutomatedPull* pull, const AutomatedPullParams& params);

}