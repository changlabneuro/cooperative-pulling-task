#include "training.hpp"
#include "common/render.hpp"

namespace om {

namespace {

template <typename State>
bool elapsed(const State* state) {
  return elapsed_time(state->t0, now()) >= state->total_time;
}

bool enter(bool* entry) {
  if (*entry) {
    *entry = false;
    return true;
  } else {
    return false;
  }
}

} //  anon

NewTrialResult tick_new_trial(NewTrialState* state, bool* entry) {
  NewTrialResult result{};
  if (enter(entry)) {
    // if (state->play_sound_on_entry) {
    //  audio::play_buffer(state->play_sound_on_entry.value(), 0.25f);
    //}
    state->t0 = now();
  }

  if (state->stim0_image) {
    gfx::draw_2d_image(state->stim0_image.value(), state->stim0_size, state->stim0_offset);
  } else {
    gfx::draw_quad(state->stim0_color, state->stim0_size, state->stim0_offset);
  }

  if (state->stim1_image) {
    gfx::draw_2d_image(state->stim1_image.value(), state->stim1_size, state->stim1_offset);
  } else {
    gfx::draw_quad(state->stim1_color, state->stim1_size, state->stim1_offset);
  }

  if (elapsed(state)) {
    result.finished = true;
  }

  return result;
}

bool tick_delay(DelayState* state, bool* entry) {
  if (enter(entry)) {
    state->t0 = now();
  }
  return elapsed(state);
}

}
