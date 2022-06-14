#pragma once

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

PullDetectResult detect_pull(PullDetect* pd, const PullDetectParams& params);

}