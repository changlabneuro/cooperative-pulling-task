#include "lever_pull.hpp"
#include "./random.hpp"
#include "./common.hpp"
#include <cassert>

namespace om::lever {

PullDetectResult detect_pull(PullDetect* pd, const PullDetectParams& params) {
    PullDetectResult result{};

    if (pd->is_high && params.current_position < pd->falling_edge) {
      pd->is_high = false;
      result.released_lever = true;

    }
    else if (!pd->is_high && params.current_position > pd->rising_edge) {
      pd->is_high = true;
      result.pulled_lever = true;

    }

      // added by Weikang - more conditions
    //else if (!pd->is_high && params.current_position < pd->falling_edge) {
     // pd->is_high = true;
     // result.released_lever = true;

    //}
   // else if (pd->is_high && params.current_position > pd->rising_edge) {
    //  pd->is_high = false;
     // result.pulled_lever = true;
    //}
 
  return result;
}

void start_automated_pull(AutomatedPull* pull, float current_force) {
  assert(pull->state == AutomatedPull::State::Idle && 
         pull->force_state == AutomatedPull::ForceTransitionState::Idle);
  *pull = {};
  pull->current_force = current_force;
  pull->state = AutomatedPull::State::Forwards0;
  pull->force_state = AutomatedPull::ForceTransitionState::Transitioning;
  pull->target_high = false;
}

AutomatedPullResult update_automated_pull(AutomatedPull* pull, const AutomatedPullParams& params) {
  AutomatedPullResult result{};
  if (pull->state == AutomatedPull::State::Idle) {
    return result;
  }

  auto now_t = now();
  om::Duration dt{};
  if (pull->state_last_time) {
    dt = now_t - pull->state_last_time.value();
  }
  pull->state_last_time = now_t;

  const float force_slope = std::max(0.0f, params.force_slope_g_s) * dt.count();
  bool finished_transition{};

  switch (pull->force_state) {
    case AutomatedPull::ForceTransitionState::Idle: {
      break;
    }
    case AutomatedPull::ForceTransitionState::Transitioning: {
      const float target_force = pull->target_high ? params.force_target_high : params.force_target_low;

      if (target_force < pull->current_force) {
        pull->current_force = std::max(target_force, float(pull->current_force - force_slope));

      } else if (target_force > pull->current_force) {
        pull->current_force = std::min(target_force, float(pull->current_force + force_slope));
      }

      if (target_force == pull->current_force) {
        pull->force_state = AutomatedPull::ForceTransitionState::Timeout;
        pull->state_elapsed_time = {};
      }

      break;
    }
    case AutomatedPull::ForceTransitionState::Timeout: {
      pull->state_elapsed_time += dt;
      if (pull->state_elapsed_time.count() > params.force_transition_timeout_s) {
        pull->force_state = AutomatedPull::ForceTransitionState::Idle;
        finished_transition = true;
      }
      break;
    }
    default: {
      assert(false);
    }
  }

  if (finished_transition) {
    switch (pull->state) {
      case AutomatedPull::State::Forwards0: {
        //  Switch to reverse direction.
        result.set_direction = false;
        pull->force_state = AutomatedPull::ForceTransitionState::Transitioning;
        pull->state = AutomatedPull::State::Reverse0;
        pull->target_high = true;
        break;
      }
      case AutomatedPull::State::Reverse0: {
        //  Set force -> 0
        pull->force_state = AutomatedPull::ForceTransitionState::Transitioning;
        pull->target_high = false;
        pull->state = AutomatedPull::State::Reverse1;
        break;
      }
      case AutomatedPull::State::Reverse1: {
        //  Switch to forward direction.
        result.set_direction = true;
        pull->force_state = AutomatedPull::ForceTransitionState::Transitioning;
        pull->state = AutomatedPull::State::Forwards1;
        pull->target_high = true;
        break;
      }
      case AutomatedPull::State::Forwards1: {
        result.elapsed = true;
        pull->state = AutomatedPull::State::Idle;
      }
    }
  }

  result.active = true;
  result.set_force = pull->current_force;

  return result;
}

om::lever::PullScheduleUpdateResult om::lever::update_pull_schedule(PullSchedule* pull) {
  om::lever::PullScheduleUpdateResult result{};

  auto curr_t = now();

  if (!pull->initialized) {
    pull->last_pull = curr_t;
    pull->began_epoch = curr_t;
    pull->initialized = true;
  }

  auto epoch_delta_t = om::Duration(curr_t - pull->began_epoch);
  if (epoch_delta_t.count() >= pull->epoch_time) {
    pull->is_downtime_epoch = !pull->is_downtime_epoch;
    pull->began_epoch = curr_t;
  }

  if (pull->is_downtime_epoch) {
    return result;
  }

  auto delta_t = om::Duration(curr_t - pull->last_pull);

  switch (pull->mode) {
    case PullSchedule::Mode::FixedInterval:
    case PullSchedule::Mode::ExpRandomInterval: {
      if (delta_t.count() >= pull->interval_s) {
        result.do_pull = true;
        pull->last_pull = curr_t;

        if (pull->mode == PullSchedule::Mode::ExpRandomInterval) {
          pull->interval_s = clamp(
            float(exprand(pull->exp_random_interval_mu)), pull->min_interval, pull->max_interval);
        }
      }
      break;
    }
    default: {
      assert(false);
    }
  }

  return result;
}

}