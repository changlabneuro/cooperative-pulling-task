#include "lever_pull.hpp"

namespace om::lever {

PullDetectResult detect_pull(PullDetect* pd, const PullDetectParams& params) {
  PullDetectResult result{};

  if (pd->is_high && params.current_position < pd->falling_edge) {
    pd->is_high = false;
    result.released_lever = true;

  } else if (!pd->is_high && params.current_position > pd->rising_edge) {
    pd->is_high = true;
    result.pulled_lever = true;
  }

  return result;
}

}