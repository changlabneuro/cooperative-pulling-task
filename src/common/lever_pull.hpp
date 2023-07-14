#pragma once

#include "time.hpp"
#include <optional>

namespace om::lever {

struct PullSchedule {
  enum class Mode {
    FixedInterval = 0,
    ExpRandomInterval = 1,
  };

  bool initialized{};
  Mode mode{Mode::ExpRandomInterval};
  float interval_s{4.0f};
  double exp_random_interval_mu{5};
  float min_interval{ 1.0f };
  float max_interval{ 30.0f };
  TimePoint last_pull{};

  bool is_downtime_epoch{};
  TimePoint began_epoch{};
  double epoch_time{ 0.0 }; // 15.0
};

struct PullScheduleUpdateResult {
  bool do_pull;
};

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
  float force_target_high{120.0f};
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

PullScheduleUpdateResult update_pull_schedule(PullSchedule* pull);

}